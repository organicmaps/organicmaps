/*******************************************************************************
 The MIT License (MIT)

 Copyright (c) 2015 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *******************************************************************************/

// This define is needed to preserve client's timestamps in events and to access
// additional data fields when processing received data on a server side.
#define ALOHALYTICS_SERVER
#include "../src/event_base.h"
#include "../src/file_manager.h"
#include "../src/gzip_wrapper.h"
#include "../src/messages_queue.h"

#include <chrono>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <utility>

namespace alohalytics {

class StatisticsReceiver {
  std::string storage_directory_;
  // Collect all data into a single file in the queue, but periodically archive it
  // and create a new one with a call to ProcessArchivedFiles
  UnlimitedFileQueue file_storage_queue_;

  // How often should we create separate files with all collected data.
  static constexpr uint64_t kArchiveFileIntervalInMS = 1000 * 60 * 60;  // One hour.
  static constexpr const char * kArchiveExtension = ".cereal";
  static constexpr const char * kGzippedArchiveExtension = ".gz";

  // Used to archive currently collected data into a separate file.
  uint64_t last_checked_time_ms_from_epoch_;

 public:
  explicit StatisticsReceiver(const std::string & storage_directory)
      : storage_directory_(storage_directory),
        last_checked_time_ms_from_epoch_(AlohalyticsBaseEvent::CurrentTimestamp()) {
    FileManager::AppendDirectorySlash(storage_directory_);
    file_storage_queue_.SetStorageDirectory(storage_directory_);
  }

  //  static std::string GenerateFileNameFromEpochMilliseconds(uint64_t ms_from_epoch) {
  //    const time_t timet = static_cast<const time_t>(ms_from_epoch / 1000);
  //    char buf[100];
  //    if (std::strftime(buf, sizeof(buf), "%F-%H%M%S", std::gmtime(&timet))) {
  //      return std::string(buf) + kArchiveExtension;
  //    } else {
  //      return std::to_string(ms_from_epoch) + kArchiveExtension;
  //    }
  //  }

  //  bool ShouldRenameFile(uint64_t current_ms_from_epoch) {
  //    last_checked_time_ms_from_epoch_ = current_ms_from_epoch;
  //    if (current_ms_from_epoch - last_checked_time_ms_from_epoch_ > kArchiveFileIntervalInMS) {
  //      last_checked_time_ms_from_epoch_ = current_ms_from_epoch;
  //      return true;
  //    }
  //    return false;
  //  }

  void ArchiveCollectedData(const std::string & destination_archive_file) {
    // TODO Should we gzip it here? Probably by calling an external tool?
    file_storage_queue_.ProcessArchivedFiles([destination_archive_file](bool, const std::string & file_path) {
      // Sanity check - this lambda should be called only once.
      try {
        if (FileManager::GetFileSize(destination_archive_file) > 0) {
          std::cerr << "ERROR in the queue? Archived file already exists: " << destination_archive_file << std::endl;
        }
      } catch (const std::exception &) {
        std::rename(file_path.c_str(), destination_archive_file.c_str());
      }
      return true;
    });
  }

  // Throws exceptions on any error.
  void ProcessReceivedHTTPBody(const std::string & gzipped_body,
                               uint64_t server_timestamp,
                               const std::string & ip,
                               const std::string & user_agent,
                               const std::string & uri) {
    // Throws GunzipErrorException.
    const std::string body = Gunzip(gzipped_body);

    std::istringstream in_stream(body);
    cereal::BinaryInputArchive in_ar(in_stream);
    std::ostringstream out_stream;
    std::unique_ptr<AlohalyticsBaseEvent> ptr;
    const std::streampos bytes_to_read = body.size();
    while (bytes_to_read > in_stream.tellg()) {
      in_ar(ptr);
      // Cereal does not check if binary data is valid. Let's do it ourselves.
      // TODO(AlexZ): Switch from Cereal to another library.
      if (ptr.get() == nullptr) {
        throw std::invalid_argument("Corrupted Cereal object, this == 0.");
      }
      // TODO(AlexZ): Looks like an overhead to cast every event instead of only the first one,
      // but what if stream contains several mixed bodies?
      const AlohalyticsIdEvent * id_event = dynamic_cast<const AlohalyticsIdEvent *>(ptr.get());
      if (id_event) {
        AlohalyticsIdServerEvent * server_id_event = new AlohalyticsIdServerEvent();
        server_id_event->timestamp = id_event->timestamp;
        server_id_event->id = id_event->id;
        server_id_event->server_timestamp = server_timestamp;
        server_id_event->ip = ip;
        server_id_event->user_agent = user_agent;
        server_id_event->uri = uri;
        ptr.reset(server_id_event);
      }
      // Serialize it back.
      cereal::BinaryOutputArchive(out_stream) << ptr;
    }
    file_storage_queue_.PushMessage(out_stream.str());
  }
};

}  // namespace alohalytics
