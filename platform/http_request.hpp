#pragma once

#include "../std/stdint.hpp"
#include "../std/function.hpp"
#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/utility.hpp"

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

  explicit HttpRequest(CallbackT onFinish, CallbackT onProgress);

public:
  virtual ~HttpRequest() = 0;

  StatusT Status() const { return m_status; }
  ProgressT Progress() const { return m_progress; }
  /// Either file path (for chunks) or downloaded data
  virtual string const & Data() const = 0;

  /// Response saved to memory buffer and retrieved with Data()
  static HttpRequest * Get(string const & url, CallbackT onFinish,
                           CallbackT onProgress = CallbackT());
  /// Content-type for request is always "application/json"
  static HttpRequest * PostJson(string const & url, string const & postData,
                                CallbackT onFinish, CallbackT onProgress = CallbackT());
  static HttpRequest * GetFile(vector<string> const & urls, string const & filePath,
                               int64_t projectedFileSize,
                               CallbackT onFinish, CallbackT onProgress = CallbackT(),
                               int64_t chunkSize = 512 * 1024);
};

bool ParseServerList(string const & jsonStr, vector<string> & outUrls);

} // namespace downloader
