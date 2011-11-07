#pragma once

#include "../std/function.hpp"
#include "../std/string.hpp"
#include "../std/list.hpp"
#include "../std/vector.hpp"
#include "../std/utility.hpp"

#include "http_request_impl_callback.hpp"

class Writer;
class HttpRequestImpl;

namespace downloader
{

/// Request will be canceled on delete
class HttpRequest : public IHttpRequestImplCallback
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

private:
  StatusT m_status;
  ProgressT m_progress;
  Writer & m_writer;
  CallbackT m_onFinish;
  CallbackT m_onProgress;

protected:
  typedef list<HttpRequestImpl *> ThreadsContainerT;
  ThreadsContainerT m_threads;

  explicit HttpRequest(Writer & writer, CallbackT onFinish, CallbackT onProgress);

  /// @name Callbacks for internal native downloading threads
  //@{
  virtual void OnSizeKnown(int64_t projectedSize);
  virtual void OnWrite(int64_t offset, void const * buffer, size_t size);
  virtual void OnFinish(long httpCode, int64_t begRange, int64_t endRange);
  //@}

public:
  virtual ~HttpRequest();

  StatusT Status() const { return m_status; }
  ProgressT Progress() const { return m_progress; }

  static HttpRequest * Get(string const & url, Writer & writer, CallbackT onFinish,
                           CallbackT onProgress = CallbackT());
  /// Content-type for request is always "application/json"
  static HttpRequest * Post(string const & url, Writer & writer, string const & postData,
                            CallbackT onFinish, CallbackT onProgress = CallbackT());
  static HttpRequest * GetChunks(vector<string> const & urls, Writer & writer, int64_t fileSize,
                                 CallbackT onFinish, CallbackT onProgress = CallbackT(),
                                 int64_t chunkSize = 512 * 1024);
};

} // namespace downloader
