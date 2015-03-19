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

// Used to avoid cereal compilation issues on iOS/MacOS when check() macro is defined.
#ifdef __APPLE__
#define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0
#endif

#include "../alohalytics.h"
#include "../http_client.h"
#include "../logger.h"
#include "../event_base.h"
#include "../gzip_wrapper.h"

#include "../cereal/include/archives/binary.hpp"
#include "../cereal/include/types/string.hpp"
#include "../cereal/include/types/map.hpp"

#define LOG_IF_DEBUG(...)                   \
  if (debug_mode_) {                        \
    alohalytics::Logger().Log(__VA_ARGS__); \
  }

namespace alohalytics {

// Used for cereal smart-pointers polymorphic serialization.
struct NoOpDeleter {
  template <typename T>
  void operator()(T*) {}
};

// Use alohalytics::Stats::Instance() to access statistics engine.
Stats::Stats() : message_queue_(*this) {}

bool Stats::UploadBuffer(const std::string& url, std::string&& buffer, bool debug_mode) {
  HTTPClientPlatformWrapper request(url);
  request.set_debug_mode(debug_mode);

  try {
    // TODO(AlexZ): Refactor FileStorageQueue to automatically append ID and gzip files, so we don't need
    // temporary memory buffer any more and files take less space.
    request.set_post_body(alohalytics::Gzip(buffer), "application/alohalytics-binary-blob", "gzip");
    return request.RunHTTPRequest() && 200 == request.error_code() && !request.was_redirected();
  } catch (const std::exception& ex) {
    if (debug_mode) {
      ALOG("Exception while trying to UploadBuffer", ex.what());
    }
  }
  return false;
}

// Processes messages passed from UI in message queue's own thread.
// TODO(AlexZ): Refactor message queue to make this method private.
void Stats::OnMessage(const std::string& message, size_t dropped_events) {
  if (dropped_events) {
    LOG_IF_DEBUG("Warning:", dropped_events, "were dropped from the queue.");
  }
  if (file_storage_queue_) {
    file_storage_queue_->PushMessage(message);
  } else {
    auto& container = memory_storage_;
    container.push_back(message);
    static const size_t kMaxEventsInMemory = 2048;
    if (container.size() > kMaxEventsInMemory) {
      container.pop_front();
      LOG_IF_DEBUG("Warning: maximum numbers of events in memory (", kMaxEventsInMemory,
                   ") was reached and the oldest one was dropped.");
    }
  }
}

// Called by file storage engine to upload file with collected data.
// Should return true if upload has been successful.
// TODO(AlexZ): Refactor FSQ to make this method private.
bool Stats::OnFileReady(const std::string& full_path_to_file) {
  if (upload_url_.empty()) {
    LOG_IF_DEBUG("Warning: upload server url was not set and file", full_path_to_file, "was not uploaded.");
    return false;
  }
  if (unique_client_id_event_.empty()) {
    LOG_IF_DEBUG("Warning: unique client ID is not set so statistics was not uploaded.");
    return false;
  }
  // TODO(AlexZ): Refactor to use crossplatform and safe/non-throwing version.
  const uint64_t file_size = bricks::FileSystem::GetFileSize(full_path_to_file);
  if (0 == file_size) {
    LOG_IF_DEBUG("ERROR: Trying to upload file of size 0?", full_path_to_file);
    return false;
  }

  LOG_IF_DEBUG("Trying to upload statistics file", full_path_to_file, "to", upload_url_);

  // Append unique installation id in the beginning of each file sent to the server.
  // It can be empty so all stats data will become anonymous.
  // TODO(AlexZ): Refactor fsq to silently add it in the beginning of each file
  // and to avoid not-enough-memory situation for bigger files.
  std::ifstream fi(full_path_to_file, std::ifstream::in | std::ifstream::binary);
  std::string buffer(unique_client_id_event_);
  const size_t id_size = unique_client_id_event_.size();
  buffer.resize(id_size + static_cast<std::string::size_type>(file_size));
  fi.read(&buffer[id_size], static_cast<std::streamsize>(file_size));
  if (fi.good()) {
    fi.close();
    return UploadBuffer(upload_url_, std::move(buffer), debug_mode_);
  } else {
    LOG_IF_DEBUG("Can't load file with size", file_size, "into memory");
  }
  return false;
}

Stats& Stats::Instance() {
  static Stats alohalytics;
  return alohalytics;
}

// Easier integration if enabled.
Stats& Stats::SetDebugMode(bool enable) {
  debug_mode_ = enable;
  LOG_IF_DEBUG("Enabled debug mode.");
  return *this;
}

// If not set, collected data will never be uploaded.
Stats& Stats::SetServerUrl(const std::string& url_to_upload_statistics_to) {
  upload_url_ = url_to_upload_statistics_to;
  LOG_IF_DEBUG("Set upload url:", url_to_upload_statistics_to);
  return *this;
}

// If not set, data will be stored in memory only.
Stats& Stats::SetStoragePath(const std::string& full_path_to_storage_with_a_slash_at_the_end) {
  LOG_IF_DEBUG("Set storage path:", full_path_to_storage_with_a_slash_at_the_end);
  auto& fsq = file_storage_queue_;
  fsq.reset(nullptr);
  if (!full_path_to_storage_with_a_slash_at_the_end.empty()) {
    fsq.reset(new TFileStorageQueue(*this, full_path_to_storage_with_a_slash_at_the_end));
    if (debug_mode_) {
      const TFileStorageQueue::Status status = fsq->GetQueueStatus();
      LOG_IF_DEBUG("Active file size:", status.appended_file_size);
      const size_t count = status.finalized.queue.size();
      if (count) {
        LOG_IF_DEBUG(count, "files with total size of", status.finalized.total_size,
                     "bytes are waiting for upload.");
      }
    }
    const size_t memory_events_count = memory_storage_.size();
    if (memory_events_count) {
      LOG_IF_DEBUG("Save", memory_events_count, "in-memory events into the file storage.");
      for (const auto& msg : memory_storage_) {
        fsq->PushMessage(msg);
      }
      memory_storage_.clear();
    }
  }
  return *this;
}

// If not set, data will be uploaded without any unique id.
Stats& Stats::SetClientId(const std::string& unique_client_id) {
  LOG_IF_DEBUG("Set unique client id:", unique_client_id);
  if (unique_client_id.empty()) {
    unique_client_id_event_.clear();
  } else {
    AlohalyticsIdEvent event;
    event.id = unique_client_id;
    std::ostringstream sstream;
    { cereal::BinaryOutputArchive(sstream) << std::unique_ptr<AlohalyticsBaseEvent, NoOpDeleter>(&event); }
    unique_client_id_event_ = sstream.str();
  }
  return *this;
}

static inline void LogEventImpl(AlohalyticsBaseEvent const& event, MessageQueue<Stats>& msq) {
  std::ostringstream sstream;
  {
    // unique_ptr is used to correctly serialize polymorphic types.
    cereal::BinaryOutputArchive(sstream) << std::unique_ptr<AlohalyticsBaseEvent const, NoOpDeleter>(&event);
  }
  msq.PushMessage(std::move(sstream.str()));
}

void Stats::LogEvent(std::string const& event_name) {
  LOG_IF_DEBUG("LogEvent:", event_name);
  AlohalyticsKeyEvent event;
  event.key = event_name;
  LogEventImpl(event, message_queue_);
}

void Stats::LogEvent(std::string const& event_name, Location const& location) {
  LOG_IF_DEBUG("LogEvent:", event_name, location.ToDebugString());
  AlohalyticsKeyLocationEvent event;
  event.key = event_name;
  event.location = location;
  LogEventImpl(event, message_queue_);
}

void Stats::LogEvent(std::string const& event_name, std::string const& event_value) {
  LOG_IF_DEBUG("LogEvent:", event_name, "=", event_value);
  AlohalyticsKeyValueEvent event;
  event.key = event_name;
  event.value = event_value;
  LogEventImpl(event, message_queue_);
}

void Stats::LogEvent(std::string const& event_name, std::string const& event_value, Location const& location) {
  LOG_IF_DEBUG("LogEvent:", event_name, "=", event_value, location.ToDebugString());
  AlohalyticsKeyValueLocationEvent event;
  event.key = event_name;
  event.value = event_value;
  event.location = location;
  LogEventImpl(event, message_queue_);
}

void Stats::LogEvent(std::string const& event_name, TStringMap const& value_pairs) {
  LOG_IF_DEBUG("LogEvent:", event_name, "=", value_pairs);
  AlohalyticsKeyPairsEvent event;
  event.key = event_name;
  event.pairs = value_pairs;
  LogEventImpl(event, message_queue_);
}

void Stats::LogEvent(std::string const& event_name, TStringMap const& value_pairs, Location const& location) {
  LOG_IF_DEBUG("LogEvent:", event_name, "=", value_pairs, location.ToDebugString());
  AlohalyticsKeyPairsLocationEvent event;
  event.key = event_name;
  event.pairs = value_pairs;
  event.location = location;
  LogEventImpl(event, message_queue_);
}

// Forcedly tries to upload all stored data to the server.
void Stats::Upload() {
  if (upload_url_.empty()) {
    LOG_IF_DEBUG("Warning: upload server url is not set, nothing was uploaded.");
    return;
  }
  if (unique_client_id_event_.empty()) {
    LOG_IF_DEBUG("Warning: unique client ID is not set so statistics was not uploaded.");
    return;
  }
  LOG_IF_DEBUG("Forcing statistics uploading.");
  if (file_storage_queue_) {
    // Upload all data, including 'current' file.
    file_storage_queue_->ForceProcessing(true);
  } else {
    std::string buffer = unique_client_id_event_;
    // TODO(AlexZ): thread-safety?
    TMemoryContainer copy;
    copy.swap(memory_storage_);
    for (const auto& evt : copy) {
      buffer.append(evt);
    }
    LOG_IF_DEBUG("Forcing in-memory statistics uploading.");
    if (!UploadBuffer(upload_url_, std::move(buffer), debug_mode_)) {
      // If failed, merge events we tried to upload with possible new ones.
      memory_storage_.splice(memory_storage_.end(), std::move(copy));
      LOG_IF_DEBUG("Failed to upload in-memory statistics.");
    }
  }
}

}  // namespace alohalytics
