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

#include <cassert>
#include <cerrno>
#include <cstdio>  // remove

#include "../alohalytics.h"
#include "../event_base.h"
#include "../file_manager.h"
#include "../gzip_wrapper.h"
#include "../http_client.h"
#include "../logger.h"

// TODO(AlexZ): Refactor out cereal library - it's too heavy overkill for us.
#include "../cereal/include/archives/binary.hpp"
#include "../cereal/include/types/string.hpp"
#include "../cereal/include/types/map.hpp"

#define LOG_IF_DEBUG(...)                   \
  if (debug_mode_) {                        \
    alohalytics::Logger().Log(__VA_ARGS__); \
  }

namespace alohalytics {

static constexpr const char * kAlohalyticsHTTPContentType = "application/alohalytics-binary-blob";

// Use alohalytics::Stats::Instance() to access statistics engine.
Stats::Stats()
    : messages_queue_(
          std::bind(&Stats::GzipAndArchiveFileInTheQueue, this, std::placeholders::_1, std::placeholders::_2)) {}

void Stats::GzipAndArchiveFileInTheQueue(const std::string & in_file, const std::string & out_archive) {
  std::string encoded_unique_client_id;
  if (unique_client_id_.empty()) {
    LOG_IF_DEBUG(
        "Warning: unique client id was not set in GzipAndArchiveFileInTheQueue,"
        "statistics will be completely anonymous and hard to process on the server.");
  } else {
    // Pre-calculation for special ID event.
    // We do it for every archived file to have a fresh timestamp.
    AlohalyticsIdEvent event;
    event.id = unique_client_id_;
    std::ostringstream ostream;
    { cereal::BinaryOutputArchive(ostream) << std::unique_ptr<AlohalyticsBaseEvent, NoOpDeleter>(&event); }
    encoded_unique_client_id = ostream.str();
  }
  LOG_IF_DEBUG("Archiving", in_file, "to", out_archive);
  // Append unique installation id in the beginning of each archived file.

  try {
    std::string buffer(std::move(encoded_unique_client_id));
    {
      std::ifstream fi;
      fi.exceptions(std::ifstream::failbit | std::ifstream::badbit);
      fi.open(in_file, std::ifstream::in | std::ifstream::binary);
      const size_t data_offset = buffer.size();
      const uint64_t file_size = FileManager::GetFileSize(in_file);
      if (file_size > std::numeric_limits<std::string::size_type>::max()
          || file_size > std::numeric_limits<std::streamsize>::max()) {
        throw std::out_of_range("File size is out of range.");
      }
      buffer.resize(data_offset + size_t(file_size));
      fi.read(&buffer[data_offset], static_cast<std::streamsize>(file_size));
    }
    {
      std::ofstream fo;
      fo.exceptions(std::ifstream::failbit | std::ifstream::badbit);
      fo.open(out_archive, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
      const std::string gzipped_buffer = Gzip(buffer);
      std::string().swap(buffer);  // Free memory.
      fo.write(gzipped_buffer.data(), gzipped_buffer.size());
    }
  } catch (const std::exception & ex) {
    LOG_IF_DEBUG("CRITICAL ERROR: Exception in GzipAndArchiveFileInTheQueue:", ex.what());
    LOG_IF_DEBUG("All data collected in", in_file, "will be lost.");
  }
  const int result = std::remove(in_file.c_str());
  if (0 != result) {
    LOG_IF_DEBUG("CRITICAL ERROR: std::remove", in_file, "has failed with error", result, "and errno", errno);
  }
}

Stats & Stats::Instance() {
  static Stats alohalytics;
  return alohalytics;
}

// Easier integration if enabled.
Stats & Stats::SetDebugMode(bool enable) {
  debug_mode_ = enable;
  LOG_IF_DEBUG("Enabled debug mode.");
  return *this;
}

Stats & Stats::SetServerUrl(const std::string & url_to_upload_statistics_to) {
  upload_url_ = url_to_upload_statistics_to;
  LOG_IF_DEBUG("Set upload url:", url_to_upload_statistics_to);
  return *this;
}

Stats & Stats::SetStoragePath(const std::string & full_path_to_storage_with_a_slash_at_the_end) {
  LOG_IF_DEBUG("Set storage path:", full_path_to_storage_with_a_slash_at_the_end);
  messages_queue_.SetStorageDirectory(full_path_to_storage_with_a_slash_at_the_end);
  return *this;
}

Stats & Stats::SetClientId(const std::string & unique_client_id) {
  LOG_IF_DEBUG("Set unique client id:", unique_client_id);
  unique_client_id_ = unique_client_id;
  return *this;
}

static inline void LogEventImpl(AlohalyticsBaseEvent const & event, THundredKilobytesFileQueue & messages_queue) {
  std::ostringstream sstream;
  {
    // unique_ptr is used to correctly serialize polymorphic types.
    cereal::BinaryOutputArchive(sstream) << std::unique_ptr<AlohalyticsBaseEvent const, NoOpDeleter>(&event);
  }
  messages_queue.PushMessage(sstream.str());
}

void Stats::LogEvent(std::string const & event_name) {
  LOG_IF_DEBUG("LogEvent:", event_name);
  AlohalyticsKeyEvent event;
  event.key = event_name;
  LogEventImpl(event, messages_queue_);
}

void Stats::LogEvent(std::string const & event_name, Location const & location) {
  LOG_IF_DEBUG("LogEvent:", event_name, location.ToDebugString());
  AlohalyticsKeyLocationEvent event;
  event.key = event_name;
  event.location = location;
  LogEventImpl(event, messages_queue_);
}

void Stats::LogEvent(std::string const & event_name, std::string const & event_value) {
  LOG_IF_DEBUG("LogEvent:", event_name, "=", event_value);
  AlohalyticsKeyValueEvent event;
  event.key = event_name;
  event.value = event_value;
  LogEventImpl(event, messages_queue_);
}

void Stats::LogEvent(std::string const & event_name, std::string const & event_value, Location const & location) {
  LOG_IF_DEBUG("LogEvent:", event_name, "=", event_value, location.ToDebugString());
  AlohalyticsKeyValueLocationEvent event;
  event.key = event_name;
  event.value = event_value;
  event.location = location;
  LogEventImpl(event, messages_queue_);
}

void Stats::LogEvent(std::string const & event_name, TStringMap const & value_pairs) {
  LOG_IF_DEBUG("LogEvent:", event_name, "=", value_pairs);
  AlohalyticsKeyPairsEvent event;
  event.key = event_name;
  event.pairs = value_pairs;
  LogEventImpl(event, messages_queue_);
}

void Stats::LogEvent(std::string const & event_name, TStringMap const & value_pairs, Location const & location) {
  LOG_IF_DEBUG("LogEvent:", event_name, "=", value_pairs, location.ToDebugString());
  AlohalyticsKeyPairsLocationEvent event;
  event.key = event_name;
  event.pairs = value_pairs;
  event.location = location;
  LogEventImpl(event, messages_queue_);
}

void Stats::Upload(TFileProcessingFinishedCallback upload_finished_callback) {
  if (upload_url_.empty()) {
    LOG_IF_DEBUG("Warning: upload server url has not been set, nothing was uploaded.");
    return;
  }
  LOG_IF_DEBUG("Trying to upload collected statistics to", upload_url_);
  messages_queue_.ProcessArchivedFiles(
      std::bind(&Stats::UploadFileImpl, this, std::placeholders::_1, std::placeholders::_2), upload_finished_callback);
}

bool Stats::UploadFileImpl(bool file_name_in_content, const std::string & content) {
  // This code should never be called if upload_url_ was not set.
  assert(!upload_url_.empty());
  HTTPClientPlatformWrapper request(upload_url_);
  request.set_debug_mode(debug_mode_);

  try {
    if (file_name_in_content) {
      request.set_body_file(content, kAlohalyticsHTTPContentType, "POST", "gzip");
    } else {
      request.set_body_data(alohalytics::Gzip(content), kAlohalyticsHTTPContentType, "POST", "gzip");
    }
    const bool uploadSucceeded = request.RunHTTPRequest() && 200 == request.error_code() && !request.was_redirected();
    LOG_IF_DEBUG("RunHTTPRequest has returned code", request.error_code(),
                 request.was_redirected() ? "and request was redirected to " + request.url_received() : " ");
    return uploadSucceeded;
  } catch (const std::exception & ex) {
    LOG_IF_DEBUG("Exception in UploadFileImpl:", ex.what());
  }
  return false;
}

}  // namespace alohalytics
