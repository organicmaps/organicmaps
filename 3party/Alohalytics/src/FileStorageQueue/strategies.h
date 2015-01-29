#ifndef FSQ_STRATEGIES_H
#define FSQ_STRATEGIES_H

#include <string>

#include "status.h"

#include "../Bricks/util/util.h"
#include "../Bricks/file/file.h"
#include "../Bricks/time/chrono.h"
#include "../Bricks/strings/fixed_size_serializer.h"

namespace fsq {
namespace strategy {

// Default file append strategy: Appends data to files in raw format, without separators.
struct JustAppendToFile {
  void AppendToFile(bricks::FileSystem::OutputFile& fo, const std::string& message) const {
    // TODO(dkorolev): Should we flush each record? Make it part of the strategy?
    fo << message << std::flush;
  }
  uint64_t MessageSizeInBytes(const std::string& message) const { return message.length(); }
};

// Another simple file append strategy: Append messages adding a separator after each of them.
class AppendToFileWithSeparator {
 public:
  void AppendToFile(bricks::FileSystem::OutputFile& fo, const std::string& message) const {
    // TODO(dkorolev): Should we flush each record? Make it part of the strategy?
    fo << message << separator_ << std::flush;
  }
  uint64_t MessageSizeInBytes(const std::string& message) const {
    return message.length() + separator_.length();
  }
  void SetSeparator(const std::string& separator) { separator_ = separator; }

 private:
  std::string separator_ = "";
};

// Default resume strategy: Always resume.
struct AlwaysResume {
  inline static bool ShouldResume() { return true; }
};

// A dummy retry strategy: Always process, no need to retry.
template <class FILE_SYSTEM>
class AlwaysProcessNoNeedToRetry {
 public:
  AlwaysProcessNoNeedToRetry(const FILE_SYSTEM&) {}
  typedef FILE_SYSTEM T_FILE_SYSTEM;
  inline void OnSuccess() {}
  inline void OnFailure() {}
  inline bool ShouldWait(bricks::time::MILLISECONDS_INTERVAL*) { return false; }
};

// Default file naming strategy: Use "finalized-{timestamp}.bin" and "current-{timestamp}.bin".
struct DummyFileNamingToUnblockAlexFromMinsk {
  struct FileNamingSchema {
    FileNamingSchema(const std::string& prefix, const std::string& suffix) : prefix_(prefix), suffix_(suffix) {}
    template <typename T_TIMESTAMP>
    inline std::string GenerateFileName(const T_TIMESTAMP timestamp) const {
      return prefix_ + bricks::strings::PackToString(timestamp) + suffix_;
    }
    template <typename T_TIMESTAMP>
    inline bool ParseFileName(const std::string& filename, T_TIMESTAMP* output_timestamp) const {
      if ((filename.length() ==
           prefix_.length() + bricks::strings::FixedSizeSerializer<T_TIMESTAMP>::size_in_bytes +
               suffix_.length()) &&
          filename.substr(0, prefix_.length()) == prefix_ &&
          filename.substr(filename.length() - suffix_.length()) == suffix_) {
        T_TIMESTAMP timestamp;
        bricks::strings::UnpackFromString(filename.substr(prefix_.length()), timestamp);
        if (GenerateFileName(timestamp) == filename) {
          *output_timestamp = timestamp;
          return true;
        } else {
          return false;
        }
      } else {
        return false;
      }
    }
    std::string prefix_;
    std::string suffix_;
  };
  FileNamingSchema current = FileNamingSchema("current-", ".bin");
  FileNamingSchema finalized = FileNamingSchema("finalized-", ".bin");
};

// Default time manager strategy: Use UNIX time in milliseconds.
struct UseEpochMilliseconds final {
  typedef bricks::time::EPOCH_MILLISECONDS T_TIMESTAMP;
  typedef bricks::time::MILLISECONDS_INTERVAL T_TIME_SPAN;
  T_TIMESTAMP Now() const { return bricks::time::Now(); }
};

// Default file finalization strategy: Keeps files under 100KB, if there are files in the processing queue,
// in case of no files waiting, keep them under 10KB. Also manage maximum age before forced finalization:
// a maximum of 24 hours when there is backlog, a maximum of 10 minutes if there is no.
template <typename TIMESTAMP,
          typename TIME_SPAN,
          uint64_t BACKLOG_MAX_FILE_SIZE,
          TIME_SPAN BACKLOG_MAX_FILE_AGE,
          uint64_t REALTIME_MAX_FILE_SIZE,
          TIME_SPAN REALTIME_MAX_FILE_AGE>
struct SimpleFinalizationStrategy {
  typedef TIMESTAMP T_TIMESTAMP;
  typedef TIME_SPAN T_TIME_SPAN;
  // This default strategy only supports MILLISECONDS from bricks:time as timestamps.
  bool ShouldFinalize(const QueueStatus<T_TIMESTAMP>& status, const T_TIMESTAMP now) const {
    if (status.appended_file_size >= BACKLOG_MAX_FILE_SIZE ||
        (now - status.appended_file_timestamp) > BACKLOG_MAX_FILE_AGE) {
      // Always keep files of at most 100KB and at most 24 hours old.
      return true;
    } else if (!status.finalized.queue.empty()) {
      // The above is the only condition as long as there are queued, pending, unprocessed files.
      return false;
    } else {
      // Otherwise, there are no files pending processing no queue,
      // and the default strategy can be legitimately expected to keep finalizing files somewhat often.
      return (status.appended_file_size >= REALTIME_MAX_FILE_SIZE ||
              (now - status.appended_file_timestamp) > REALTIME_MAX_FILE_AGE);
    }
  }
};

typedef SimpleFinalizationStrategy<bricks::time::EPOCH_MILLISECONDS,
                                   bricks::time::MILLISECONDS_INTERVAL,
                                   100 * 1024,
                                   bricks::time::MILLISECONDS_INTERVAL(24 * 60 * 60 * 1000),
                                   10 * 1024,
                                   bricks::time::MILLISECONDS_INTERVAL(10 * 60 * 1000)>
    KeepFilesAround100KBUnlessNoBacklog;

// Default file purge strategy: Keeps under 1K files of under 20MB of total size.
template <uint64_t MAX_TOTAL_SIZE, size_t MAX_FILES>
struct SimplePurgeStrategy {
  template <typename T_TIMESTAMP>
  bool ShouldPurge(const QueueStatus<T_TIMESTAMP>& status) const {
    if (status.finalized.total_size + status.appended_file_size > MAX_TOTAL_SIZE) {
      // Purge the oldest files if the total size of data stored in the queue exceeds MAX_TOTAL_SIZE.
      return true;
    } else if (status.finalized.queue.size() > MAX_FILES) {
      // Purge the oldest files if the total number of queued files exceeds MAX_FILE.
      return true;
    } else {
      // Good to go otherwise.
      return false;
    }
  }
};

typedef SimplePurgeStrategy<20 * 1024 * 1024, 1000> KeepUnder20MBAndUnder1KFiles;

}  // namespace strategy
}  // namespace fsq

#endif  // FSQ_STRATEGIES_H
