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

#ifndef ALOHALYTICS_H
#define ALOHALYTICS_H

#include "location.h"
#include "messages_queue.h"

#include <string>
#include <map>
#include <list>
#include <memory>

namespace alohalytics {

typedef std::map<std::string, std::string> TStringMap;

class Stats final {
  // Is statistics engine enabled or disabled.
  // Used if users want to opt-out from events collection.
  bool enabled_ = true;
  std::string upload_url_;
  // Unique client id is inserted as a special event in the beginning of every archived file before gzipping it.
  // In current implementation it is used to distinguish between different users in the events stream on the server.
  // NOTE: Statistics will not be uploaded if unique client id was not set.
  std::string unique_client_id_;
  THundredKilobytesFileQueue messages_queue_;
  bool debug_mode_ = false;

  // Use alohalytics::Stats::Instance() to access statistics engine.
  Stats();

  // Should return false on upload error.
  bool UploadFileImpl(bool file_name_in_content, const std::string & content);

  // Called by the queue when file size limit was hit or immediately before file is sent to a server.
  // in_file will be:
  // - Gzipped.
  // - Saved as out_archive for easier post-processing (e.g. uploading).
  // - Deleted.
  void GzipAndArchiveFileInTheQueue(const std::string & in_file, const std::string & out_archive);

 public:
  static Stats & Instance();

  // Easier integration if enabled.
  Stats & SetDebugMode(bool enable);
  bool DebugMode() const { return debug_mode_; }

  // Turn off events collection and sending.
  void Disable();
  // Turn back on events collection and sending after Disable();
  void Enable();

  // If not set, collected data will never be uploaded.
  Stats & SetServerUrl(const std::string & url_to_upload_statistics_to);

  // If not set, data will be stored in memory only.
  Stats & SetStoragePath(const std::string & full_path_to_storage_with_a_slash_at_the_end);

  // If not set, data will never be uploaded.
  // TODO(AlexZ): Should we allow anonymous statistics uploading?
  Stats & SetClientId(const std::string & unique_client_id);

  void LogEvent(std::string const & event_name);
  void LogEvent(std::string const & event_name, Location const & location);

  void LogEvent(std::string const & event_name, std::string const & event_value);
  void LogEvent(std::string const & event_name, std::string const & event_value, Location const & location);

  void LogEvent(std::string const & event_name, TStringMap const & value_pairs);
  void LogEvent(std::string const & event_name, TStringMap const & value_pairs, Location const & location);

  // Uploads all previously collected data to the server.
  void Upload(TFileProcessingFinishedCallback upload_finished_callback = TFileProcessingFinishedCallback());
};

inline void LogEvent(std::string const & event_name) { Stats::Instance().LogEvent(event_name); }
inline void LogEvent(std::string const & event_name, Location const & location) {
  Stats::Instance().LogEvent(event_name, location);
}

inline void LogEvent(std::string const & event_name, std::string const & event_value) {
  Stats::Instance().LogEvent(event_name, event_value);
}
inline void LogEvent(std::string const & event_name, std::string const & event_value, Location const & location) {
  Stats::Instance().LogEvent(event_name, event_value, location);
}

inline void LogEvent(std::string const & event_name, TStringMap const & value_pairs) {
  Stats::Instance().LogEvent(event_name, value_pairs);
}
inline void LogEvent(std::string const & event_name, TStringMap const & value_pairs, Location const & location) {
  Stats::Instance().LogEvent(event_name, value_pairs, location);
}

}  // namespace alohalytics

#endif  // #ifndef ALOHALYTICS_H
