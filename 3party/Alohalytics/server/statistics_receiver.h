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
#include "../src/gzip_wrapper.h"
#include "../src/messages_queue.h"

#include <sstream>
#include <utility>

namespace alohalytics {

class StatisticsReceiver {
  std::string storage_directory_;
  TUnlimitedFileQueue file_storage_queue_;

 public:
  explicit StatisticsReceiver(const std::string & storage_directory) : storage_directory_(storage_directory) {
    FileManager::AppendDirectorySlash(storage_directory_);
    file_storage_queue_.SetStorageDirectory(storage_directory_);
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
      if (!ptr) {
        throw std::invalid_argument("Corrupted Cereal object, this == 0.");
      }
      // TODO(AlexZ): Looks like an overhead to cast every event instead of only the first one,
      // but what if stream contains several mixed bodies?
      const AlohalyticsIdEvent * id_event = dynamic_cast<const AlohalyticsIdEvent *>(ptr.get());
      if (id_event) {
        std::unique_ptr<AlohalyticsIdServerEvent> server_id_event(new AlohalyticsIdServerEvent());
        server_id_event->timestamp = id_event->timestamp;
        server_id_event->id = id_event->id;
        server_id_event->server_timestamp = server_timestamp;
        server_id_event->ip = ip;
        server_id_event->user_agent = user_agent;
        server_id_event->uri = uri;
        ptr = std::move(server_id_event);
      }
      // Serialize it back.
      cereal::BinaryOutputArchive(out_stream) << ptr;
    }
    file_storage_queue_.PushMessage(out_stream.str());
  }

  // Correct logrotate utility support for queue's file.
  void ReopenDataFile() { file_storage_queue_.LogrotateCurrentFile(); }
};

}  // namespace alohalytics
