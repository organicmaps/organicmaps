#pragma once

#include "../std/function.hpp"
#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/utility.hpp"
#include "../std/scoped_ptr.hpp"

class Writer;

namespace downloader
{

/// Request in progress will be canceled on delete
class HttpRequest
{
public:
  enum StatusT
  {
    EInProgress,
    ECompleted,
    EFailed
  };

  /// <current, total>, total can be -1 if size is unknown
  typedef pair<int64_t, int64_t> ProgressT;
  typedef function<void(HttpRequest &)> CallbackT;

protected:
  StatusT m_status;
  ProgressT m_progress;
  CallbackT m_onFinish;
  CallbackT m_onProgress;
  string m_data;
  scoped_ptr<Writer> m_writer;

  explicit HttpRequest(string const & filePath, CallbackT onFinish, CallbackT onProgress);

public:
  virtual ~HttpRequest() = 0;

  StatusT Status() const { return m_status; }
  ProgressT Progress() const { return m_progress; }
  /// Retrieve either file path or downloaded data
  string const & Data() const { return m_data; }

  /// @param[in] filePath if empty, request will be saved to memory, see Data()
  static HttpRequest * Get(string const & url, string const & filePath, CallbackT onFinish,
                           CallbackT onProgress = CallbackT());
  /// Content-type for request is always "application/json"
  static HttpRequest * Post(string const & url, string const & filePath, string const & postData,
                            CallbackT onFinish, CallbackT onProgress = CallbackT());
  static HttpRequest * GetChunks(vector<string> const & urls, string const & filePath, int64_t fileSize,
                                 CallbackT onFinish, CallbackT onProgress = CallbackT(),
                                 int64_t chunkSize = 512 * 1024);
};

} // namespace downloader
