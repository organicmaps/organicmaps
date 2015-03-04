// Class FSQ manages local, filesystem-based message queue.
//
// A temporary append-only file is created and then written into. Once the strategy dictates so,
// it is declared finalized and gets atomically renamed to a different name (with its 1st timestamp in it),
// using which name it is passed to the PROCESSOR. A new new append-only file is started in the meantime.
//
// The processor runs in a dedicated thread. Thus, it is guaranteed to process at most one file at a time.
// It can take as long as it needs to process the file. Files are guaranteed to be passed in the FIFO order.
//
// Once a file is ready, which translates to "on startup" if there are pending files,
// the user handler in PROCESSOR::OnFileReady(file_name) is invoked.
// When a retry strategy is active, further logic depends on the return value of this method,
// see the description of the `FileProcessingResult` enum below for more details.
//
// On top of the above FSQ keeps an eye on the size it occupies on disk and purges the oldest data files
// if the specified purge strategy dictates so.

#ifndef FSQ_H
#define FSQ_H

#include <algorithm>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "status.h"
#include "exception.h"
#include "config.h"
#include "strategies.h"

#include "../Bricks/file/file.h"
#include "../Bricks/time/chrono.h"

namespace fsq {

// On `Success`, FQS deleted file that just got processed and sends the next one to as it arrives,
// which can happen immediately, if the queue is not empty, or later, once the next file is ready.
//
// On `SuccessAndMoved`, FQS does the same thing as for `Success`, except for it does not attempt
// to delete the file, assuming that it has already been deleted or otherwise taken care of by the user code.
// Keep in mind that the user code is responsible for making sure the file is removed or renamed,
// otherwise it will be re-processed after the application restarts, because of matching the mask.
//
// On `Unavailable`, automatic file processing is suspended until it is resumed externally.
// An example of this case would be the processor being the file uploader, with the device going offline.
// This way, no further action is required until FQS is explicitly notified that the device is back online.
//
// On `FailureNeedRetry`, the file is kept and will be re-attempted to be sent to the processor,
// with respect to the retry strategy specified as the template parameter to FSQ.
enum class FileProcessingResult { Success, SuccessAndMoved, Unavailable, FailureNeedRetry };

template <class CONFIG>
class FSQ final : public CONFIG::T_FILE_NAMING_STRATEGY,
                  public CONFIG::T_FINALIZE_STRATEGY,
                  public CONFIG::T_PURGE_STRATEGY,
                  public CONFIG::T_FILE_APPEND_STRATEGY,
                  public CONFIG::template T_RETRY_STRATEGY<typename CONFIG::T_FILE_SYSTEM> {
 public:
  typedef CONFIG T_CONFIG;

  typedef typename T_CONFIG::T_PROCESSOR T_PROCESSOR;
  typedef typename T_CONFIG::T_MESSAGE T_MESSAGE;
  typedef typename T_CONFIG::T_FILE_APPEND_STRATEGY T_FILE_APPEND_STRATEGY;
  typedef typename T_CONFIG::T_FILE_RESUME_STRATEGY T_FILE_RESUME_STRATEGY;
  typedef typename T_CONFIG::T_FILE_NAMING_STRATEGY T_FILE_NAMING_STRATEGY;
  template <typename FILE_SYSTEM>
  using T_RETRY_STRATEGY = typename T_CONFIG::template T_RETRY_STRATEGY<FILE_SYSTEM>;
  typedef T_RETRY_STRATEGY<typename CONFIG::T_FILE_SYSTEM> T_RETRY_STRATEGY_INSTANCE;
  typedef typename T_CONFIG::T_FILE_SYSTEM T_FILE_SYSTEM;
  typedef typename T_CONFIG::T_TIME_MANAGER T_TIME_MANAGER;
  typedef typename T_CONFIG::T_FINALIZE_STRATEGY T_FINALIZE_STRATEGY;
  typedef typename T_CONFIG::T_PURGE_STRATEGY T_PURGE_STRATEGY;

  typedef typename T_TIME_MANAGER::T_TIMESTAMP T_TIMESTAMP;
  typedef typename T_TIME_MANAGER::T_TIME_SPAN T_TIME_SPAN;

  typedef QueueFinalizedFilesStatus<T_TIMESTAMP> FinalizedFilesStatus;
  typedef QueueStatus<T_TIMESTAMP> Status;

  // The constructor initializes all the parameters and starts the worker thread.
  FSQ(T_PROCESSOR& processor,
      const std::string& working_directory,
      const T_TIME_MANAGER& time_manager,
      const T_FILE_SYSTEM& file_system,
      const T_RETRY_STRATEGY_INSTANCE& retry_strategy)
      : T_RETRY_STRATEGY_INSTANCE(retry_strategy),
        processor_(processor),
        working_directory_(working_directory),
        time_manager_(time_manager),
        file_system_(file_system) {
    T_CONFIG::Initialize(*this);
    worker_thread_ = std::thread(&FSQ::WorkerThread, this);
  }
  FSQ(T_PROCESSOR& processor,
      const std::string& working_directory,
      const T_TIME_MANAGER& time_manager = T_TIME_MANAGER(),
      const T_FILE_SYSTEM& file_system = T_FILE_SYSTEM())
      : FSQ(processor, working_directory, time_manager, file_system, T_RETRY_STRATEGY_INSTANCE(file_system)) {}

  // Destructor gracefully terminates worker thread and optionally joins it.
  ~FSQ() {
    // Notify the worker thread that it's time to wrap up.
    {
      std::unique_lock<std::mutex> lock(status_mutex_);
      force_worker_thread_shutdown_ = true;
      queue_status_condition_variable_.notify_all();
    }
    // Close the current file. `current_file_.reset(nullptr);` is always safe especially in destructor.
    current_file_.reset(nullptr);
    // Either wait for the processor thread to terminate or detach it, unless it's already done.
    if (worker_thread_.joinable()) {
      if (T_CONFIG::DetachProcessingThreadOnTermination()) {
        worker_thread_.detach();
      } else {
        worker_thread_.join();
      }
    }
  }

  // Getters.
  const std::string& WorkingDirectory() const { return working_directory_; }

  const Status GetQueueStatus() const {
    if (!status_ready_) {
      std::unique_lock<std::mutex> lock(status_mutex_);
      while (!status_ready_) {
        queue_status_condition_variable_.wait(lock);
        if (force_worker_thread_shutdown_) {
          throw FSQException();
        }
      }
    }
    // Returning `status_` by const reference is not thread-safe, return a copy from a locked section.
    return status_;
  }

  // `PushMessage()` appends data to the queue.
  void PushMessage(const T_MESSAGE& message) {
    if (!status_ready_) {
      // Need to wait for the status to be ready, otherwise current file resume might not happen.
      std::unique_lock<std::mutex> lock(status_mutex_);
      while (!status_ready_) {
        queue_status_condition_variable_.wait(lock);
      }
    }
    if (force_worker_thread_shutdown_) {
      if (T_CONFIG::NoThrowOnPushMessageWhileShuttingDown()) {
        // Silently ignoring incoming messages while in shutdown mode is the default strategy.
        return;
      } else {
        throw FSQException();
      }
    } else {
      const T_TIMESTAMP now = time_manager_.Now();
      const uint64_t message_size_in_bytes = T_FILE_APPEND_STRATEGY::MessageSizeInBytes(message);
      {
        // Take current message size into consideration when making file finalization decision.
        status_.appended_file_size += message_size_in_bytes;
        const bool should_finalize = T_FINALIZE_STRATEGY::ShouldFinalize(status_, now);
        status_.appended_file_size -= message_size_in_bytes;
        if (should_finalize) {
          FinalizeCurrentFile();
        }
      }
      EnsureCurrentFileIsOpen(now);
      if (!current_file_ || current_file_->bad()) {
        throw FSQException();
      }
      T_FILE_APPEND_STRATEGY::AppendToFile(*current_file_.get(), message);
      status_.appended_file_size += message_size_in_bytes;
      if (T_FINALIZE_STRATEGY::ShouldFinalize(status_, now)) {
        FinalizeCurrentFile();
      }
    }
  }

  // `ResumeProcessing() is used when a temporary reason of unavailability is now gone.
  // A common usecase is if the processor sends files over network, and the network just became unavailable.
  // In this case, on an event of network becoming available again, `ResumeProcessing()` should be called.
  //
  // `ResumeProcessing()` respects retransmission delays strategy. While the "Unavailability" event
  // does not trigger retransmission logic, if, due to prior external factors, FSQ is in waiting mode
  // for a while,
  // `ResumeProcessing()` would not override that wait. Use `ForceProcessing()` for those forced overrides.
  void ResumeProcessing() {
    processing_suspended_ = false;
    queue_status_condition_variable_.notify_all();
  }

  // `ForceProcessing()` initiates processing of finalized files, if any.
  //
  // THIS METHOD IS NOT SAFE, since using it based on a frequent external event, like "WiFi connected",
  // may result in file queue processing to be started and re-started multiple times,
  // skipping the retransmission delay logic.
  //
  // Only use `ForceProcessing()` when processing should start no matter what.
  // Example: App just got updated, or a large external download has just been successfully completed.
  //
  // Use `ResumeProcessing()` in other cases.
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // TODO(AlexZ): Refactor it as this method fails sometimes when force_finalize_current_file == true and when it is called from the main app thread.
  // As a result, after calling PushMessage(urgentEvent); ForceProcessing(true); urgentEvent is either lost or not sent to the server even if connection was available at that moment.
  // Some facts: current_file_name_ is empty and status_.appended_file_timestamp is 0 at that time.
  void ForceProcessing(bool force_finalize_current_file = false) {
    std::unique_lock<std::mutex> lock(status_mutex_);
    if (force_finalize_current_file || status_.finalized.queue.empty()) {
      if (current_file_) {
        FinalizeCurrentFile(lock);
      }
    }
    processing_suspended_ = false;
    force_processing_ = true;
    queue_status_condition_variable_.notify_all();
  }

  // `FinalizeCurrentFile()` forces the finalization of the currently appended file.
  void FinalizeCurrentFile() {
    if (current_file_) {
      std::unique_lock<std::mutex> lock(status_mutex_);
      FinalizeCurrentFile(lock);
    }
  }

  // Removes all finalized and current files from disk.
  // Has to shut down as well, since removing files does not play well with the worker thread processing them.
  // USE CAREFULLY!
  void ShutdownAndRemoveAllFSQFiles() {
    // First, force the worker thread to terminate.
    {
      std::unique_lock<std::mutex> lock(status_mutex_);
      force_worker_thread_shutdown_ = true;
      queue_status_condition_variable_.notify_all();
    }
    current_file_.reset(nullptr);
    worker_thread_.join();
    // Scan the directory and remove the files.
    for (const auto& file : ScanDir([this](const std::string& s, T_TIMESTAMP* t) {
           return T_FILE_NAMING_STRATEGY::finalized.ParseFileName(s, t) ||
                  T_FILE_NAMING_STRATEGY::current.ParseFileName(s, t);
         })) {
      T_FILE_SYSTEM::RemoveFile(file.full_path_name);
    }
  }

 private:
  // If the current file exists, declare it finalized, rename it under a permanent name
  // and notify the worker thread that a new file is available.
  void FinalizeCurrentFile(std::unique_lock<std::mutex>& already_acquired_status_mutex_lock) {
    if (current_file_) {
      current_file_.reset(nullptr);
      const std::string finalized_file_name =
          T_FILE_NAMING_STRATEGY::finalized.GenerateFileName(status_.appended_file_timestamp);
      FileInfo<T_TIMESTAMP> finalized_file_info(
          finalized_file_name,
          T_FILE_SYSTEM::JoinPath(working_directory_, finalized_file_name),
          status_.appended_file_timestamp,
          status_.appended_file_size);
      T_FILE_SYSTEM::RenameFile(current_file_name_, finalized_file_info.full_path_name);
      status_.finalized.queue.push_back(finalized_file_info);
      status_.finalized.total_size += status_.appended_file_size;
      status_.appended_file_size = 0;
      status_.appended_file_timestamp = T_TIMESTAMP(0);
      current_file_name_.clear();
      PurgeFilesAsNecessary(already_acquired_status_mutex_lock);
      queue_status_condition_variable_.notify_all();
    }
  }

  // Scans the directory for the files that match certain predicate.
  // Gets their sized and and extracts timestamps from their names along the way.
  template <typename F>
  std::vector<FileInfo<T_TIMESTAMP>> ScanDir(F f) const {
    std::vector<FileInfo<T_TIMESTAMP>> matched_files_list;
    const auto& dir = working_directory_;
    T_FILE_SYSTEM::ScanDir(working_directory_,
                           [this, &matched_files_list, &f, dir](const std::string& file_name) {
      // if (T_FILE_NAMING_STRATEGY::template IsFinalizedFileName<T_TIMESTAMP>(file_name)) {
      T_TIMESTAMP timestamp;
      if (f(file_name, &timestamp)) {
        matched_files_list.emplace_back(file_name,
                                        T_FILE_SYSTEM::JoinPath(working_directory_, file_name),
                                        timestamp,
                                        T_FILE_SYSTEM::GetFileSize(T_FILE_SYSTEM::JoinPath(dir, file_name)));
      }
    });
    std::sort(matched_files_list.begin(), matched_files_list.end());
    return matched_files_list;
  }

  // EnsureCurrentFileIsOpen() expires the current file and/or creates the new one as necessary.
  void EnsureCurrentFileIsOpen(const T_TIMESTAMP now) {
    if (!current_file_) {
      current_file_name_ =
          T_FILE_SYSTEM::JoinPath(working_directory_, T_FILE_NAMING_STRATEGY::current.GenerateFileName(now));
      // TODO(dkorolev): This relies on OutputFile being std::ofstream. Fine for now anyway.
      current_file_.reset(new typename T_FILE_SYSTEM::OutputFile(current_file_name_,
                                                                 std::ofstream::trunc | std::ofstream::binary));
      status_.appended_file_timestamp = now;
    }
  }

  // Purges the old files as necessary.
  void PurgeFilesAsNecessary(std::unique_lock<std::mutex>& already_acquired_status_mutex_lock) {
    static_cast<void>(already_acquired_status_mutex_lock);
    while (!status_.finalized.queue.empty() && T_PURGE_STRATEGY::ShouldPurge(status_)) {
      const std::string filename = status_.finalized.queue.front().full_path_name;
      status_.finalized.total_size -= status_.finalized.queue.front().size;
      status_.finalized.queue.pop_front();
      T_FILE_SYSTEM::RemoveFile(filename);
    }
  }

  // The worker thread first scans the directory for present finalized and current files.
  // Present finalized files are queued up.
  // If more than one present current files is available, all but one are finalized on the spot.
  // The one remaining current file can be appended to or finalized depending on the strategy.
  void WorkerThread() {
    // Step 1/4: Get the list of finalized files.
    typedef std::vector<FileInfo<T_TIMESTAMP>> FileInfoVector;
    const FileInfoVector& finalized_files_on_disk = ScanDir([this](const std::string& s, T_TIMESTAMP* t) {
      return this->finalized.ParseFileName(s, t);
    });
    status_.finalized.queue.assign(finalized_files_on_disk.begin(), finalized_files_on_disk.end());
    status_.finalized.total_size = 0;
    for (const auto& file : finalized_files_on_disk) {
      status_.finalized.total_size += file.size;
    }

    // Step 2/4: Get the list of current files.
    const FileInfoVector& current_files_on_disk = ScanDir([this](
        const std::string& s, T_TIMESTAMP* t) { return this->current.ParseFileName(s, t); });
    if (!current_files_on_disk.empty()) {
      const bool resume = T_FILE_RESUME_STRATEGY::ShouldResume();
      const size_t number_of_files_to_finalize = current_files_on_disk.size() - (resume ? 1u : 0u);
      for (size_t i = 0; i < number_of_files_to_finalize; ++i) {
        const FileInfo<T_TIMESTAMP>& f = current_files_on_disk[i];
        const std::string finalized_file_name = T_FILE_NAMING_STRATEGY::finalized.GenerateFileName(f.timestamp);
        FileInfo<T_TIMESTAMP> finalized_file_info(
            finalized_file_name,
            T_FILE_SYSTEM::JoinPath(working_directory_, finalized_file_name),
            f.timestamp,
            f.size);
        T_FILE_SYSTEM::RenameFile(f.full_path_name, finalized_file_info.full_path_name);
        status_.finalized.queue.push_back(finalized_file_info);
        status_.finalized.total_size += f.size;
      }
      if (resume) {
        const FileInfo<T_TIMESTAMP>& c = current_files_on_disk.back();
        status_.appended_file_timestamp = c.timestamp;
        status_.appended_file_size = c.size;
        current_file_name_ = c.full_path_name;
        // TODO(dkorolev): This relies on OutputFile being std::ofstream. Fine for now anyway.
        current_file_.reset(new typename T_FILE_SYSTEM::OutputFile(current_file_name_,
                                                                   std::ofstream::app | std::ofstream::binary));
      }
      std::unique_lock<std::mutex> lock(status_mutex_);
      PurgeFilesAsNecessary(lock);
    }

    // Step 3/4: Signal that FSQ's status has been successfully parsed from disk and FSQ is ready to go.
    {
      std::unique_lock<std::mutex> lock(status_mutex_);
      status_ready_ = true;
      queue_status_condition_variable_.notify_all();
    }

    // Step 4/4: Start processing finalized files via T_PROCESSOR, respecting retry strategy.
    while (true) {
      // Wait for a newly arrived file or another event to happen.
      std::unique_ptr<FileInfo<T_TIMESTAMP>> next_file;
      {
        std::unique_lock<std::mutex> lock(status_mutex_);
        const bricks::time::EPOCH_MILLISECONDS begin_ms = bricks::time::Now();
        bricks::time::MILLISECONDS_INTERVAL wait_ms;
        const bool should_wait = T_RETRY_STRATEGY_INSTANCE::ShouldWait(&wait_ms);
        const auto predicate = [this, should_wait, begin_ms, wait_ms]() {
          if (force_worker_thread_shutdown_) {
            return true;
          } else if (force_processing_) {
            return true;
          } else if (processing_suspended_) {
            return false;
          } else if (should_wait && bricks::time::Now() - begin_ms < wait_ms) {
            return false;
          } else if (!status_.finalized.queue.empty()) {
            return true;
          } else {
            return false;
          }
        };
        if (!predicate()) {
          if (should_wait) {
            // Add one millisecond to avoid multiple runs of this loop when `wait_ms` is close to zero.
            queue_status_condition_variable_.wait_for(
                lock, std::chrono::milliseconds(static_cast<uint64_t>(wait_ms) + 1), predicate);
          } else {
            queue_status_condition_variable_.wait(lock, predicate);
          }
        }
        if (!status_.finalized.queue.empty()) {
          next_file.reset(new FileInfo<T_TIMESTAMP>(status_.finalized.queue.front()));
        }
        if (force_worker_thread_shutdown_) {
          // By default, terminate immediately.
          // However, allow the user to override this setting and have the queue
          // processed in full before returning from FSQ's destructor.
          if (!T_CONFIG::ProcessQueueToTheEndOnShutdown() || !next_file) {
            return;
          }
        }
      }

      // Process the file, if available.
      if (next_file) {
        //      const FileProcessingResult result = processor_.OnFileReady(*next_file.get(),
        //      time_manager_.Now());
        // AlexZ: we can't trust next_file->size here. Debugging shows that it can be greater than the real file size.
        // TODO: refactor FSQ or capture a bug.
        const bool successfully_processed = processor_.OnFileReady(next_file->full_path_name);
        // Important to clear force_processing_, in a locked way.
        {
          std::unique_lock<std::mutex> lock(status_mutex_);
          force_processing_ = false;
        }
        //      if (result == FileProcessingResult::Success || result == FileProcessingResult::SuccessAndMoved)
        //      {
        if (successfully_processed) {
          std::unique_lock<std::mutex> lock(status_mutex_);
          processing_suspended_ = false;
          if (*next_file.get() == status_.finalized.queue.front()) {
            status_.finalized.total_size -= status_.finalized.queue.front().size;
            status_.finalized.queue.pop_front();
          } else {
            // The `front()` part of the queue should only be altered by this worker thread.
            throw FSQException();
          }
          //        if (result == FileProcessingResult::Success) {
          T_FILE_SYSTEM::RemoveFile(next_file->full_path_name);
          //        }
          T_RETRY_STRATEGY_INSTANCE::OnSuccess();
          //      } else if (result == FileProcessingResult::Unavailable) {
          //        std::unique_lock<std::mutex> lock(status_mutex_);
          //        processing_suspended_ = true;
        } else {  // if (result == FileProcessingResult::FailureNeedRetry) {
          T_RETRY_STRATEGY_INSTANCE::OnFailure();
        }  // else {
        // throw FSQException();
        //}
      }
    }
  }

  Status status_;
  // Appending messages is single-threaded and thus lock-free.
  // The status of the processing queue, on the other hand, should be guarded.
  mutable std::mutex status_mutex_;
  // Set to true and pings the variable once the initial directory scan is completed.
  bool status_ready_ = false;
  mutable std::condition_variable queue_status_condition_variable_;

  T_PROCESSOR& processor_;
  std::string working_directory_;
  const T_TIME_MANAGER& time_manager_;
  const T_FILE_SYSTEM& file_system_;

  std::unique_ptr<typename T_FILE_SYSTEM::OutputFile> current_file_;
  std::string current_file_name_;

  bool processing_suspended_ = false;
  bool force_processing_ = false;
  bool force_worker_thread_shutdown_ = false;

  // `worker_thread_` should be the last class member, since it should be initialized last.
  std::thread worker_thread_;

  FSQ(const FSQ&) = delete;
  FSQ(FSQ&&) = delete;
  void operator=(const FSQ&) = delete;
  void operator=(FSQ&&) = delete;
};

}  // namespace fsq

#endif  // FSQ_H
