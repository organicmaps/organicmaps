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

#include "message_queue.h"
#include "FileStorageQueue/fsq.h"
#include "location.h"

#include <string>
#include <map>
#include <list>
#include <memory>

namespace alohalytics {

typedef std::map<std::string, std::string> TStringMap;

class MQMessage {
  std::string message_;
  bool force_upload_;
public:
  MQMessage(std::string&& msg) : message_(std::move(msg)), force_upload_(false) {}
  explicit MQMessage(bool force_upload = false) : force_upload_(force_upload) {}
  // True for special empty message which should force stats uploading.
  bool ForceUpload() const { return force_upload_; }
  std::string const & GetMessage() const { return message_; }
};

class Stats final {
  std::string upload_url_;
  // Stores already serialized and ready-to-append event with unique client id.
  // NOTE: Statistics will not be uploaded if unique client id was not set.
  std::string unique_client_id_event_;
  MessageQueue<Stats, MQMessage> message_queue_;
  typedef fsq::FSQ<fsq::Config<Stats>> TFileStorageQueue;
  // TODO(AlexZ): Refactor storage queue so it can store messages in memory if no file directory was set.
  std::unique_ptr<TFileStorageQueue> file_storage_queue_;
  // Used to store events if no storage path was set.
  // Flushes all data to file storage and is not used any more if storage path was set.
  typedef std::list<std::string> TMemoryContainer;
  TMemoryContainer memory_storage_;
  bool debug_mode_ = false;

  // Use alohalytics::Stats::Instance() to access statistics engine.
  Stats();

  static bool UploadBuffer(const std::string& url, std::string&& buffer, bool debug_mode);

 public:
  // Processes messages passed from UI in message queue's own thread.
  // TODO(AlexZ): Refactor message queue to make this method private.
  void OnMessage(const MQMessage& message, size_t dropped_events);

  // Called by file storage engine to upload file with collected data.
  // Should return true if upload has been successful.
  // TODO(AlexZ): Refactor FSQ to make this method private.
  bool OnFileReady(const std::string& full_path_to_file);

  static Stats& Instance();

  // Easier integration if enabled.
  Stats& SetDebugMode(bool enable);

  // If not set, collected data will never be uploaded.
  Stats& SetServerUrl(const std::string& url_to_upload_statistics_to);

  // If not set, data will be stored in memory only.
  Stats& SetStoragePath(const std::string& full_path_to_storage_with_a_slash_at_the_end);

  // If not set, data will be uploaded without any unique id.
  Stats& SetClientId(const std::string& unique_client_id);

  void LogEvent(std::string const& event_name);
  void LogEvent(std::string const& event_name, Location const& location);

  void LogEvent(std::string const& event_name, std::string const& event_value);
  void LogEvent(std::string const& event_name, std::string const& event_value, Location const& location);

  void LogEvent(std::string const& event_name, TStringMap const& value_pairs);
  void LogEvent(std::string const& event_name, TStringMap const& value_pairs, Location const& location);

  // Forcedly tries to upload all stored data to the server.
  void Upload();
};

inline void LogEvent(std::string const& event_name) { Stats::Instance().LogEvent(event_name); }
inline void LogEvent(std::string const& event_name, Location const& location) {
  Stats::Instance().LogEvent(event_name, location);
}

inline void LogEvent(std::string const& event_name, std::string const& event_value) {
  Stats::Instance().LogEvent(event_name, event_value);
}
inline void LogEvent(std::string const& event_name, std::string const& event_value, Location const& location) {
  Stats::Instance().LogEvent(event_name, event_value, location);
}

inline void LogEvent(std::string const& event_name, TStringMap const& value_pairs) {
  Stats::Instance().LogEvent(event_name, value_pairs);
}
inline void LogEvent(std::string const& event_name, TStringMap const& value_pairs, Location const& location) {
  Stats::Instance().LogEvent(event_name, value_pairs, location);
}

}  // namespace alohalytics

#endif  // #ifndef ALOHALYTICS_H
