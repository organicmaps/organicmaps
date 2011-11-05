#include "http_request.hpp"

#ifdef DEBUG
  #include "../base/thread.hpp"
#endif

#include "../coding/writer.hpp"

namespace downloader
{

/// @return 0 if creation failed
HttpRequestImpl * CreateNativeHttpRequest(string const & url,
                                          IHttpRequestImplCallback & callback,
                                          int64_t begRange = 0,
                                          int64_t endRange = -1,
                                          string const & postBody = string());
void DeleteNativeHttpRequest(HttpRequestImpl * request);


HttpRequest::HttpRequest(Writer & writer, CallbackT onFinish, CallbackT onProgress)
  : m_status(EInProgress), m_progress(make_pair(-1, -1)), m_writer(writer),
    m_onFinish(onFinish), m_onProgress(onProgress)
{
}

HttpRequest::~HttpRequest()
{
  for (ThreadsContainerT::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
     DeleteNativeHttpRequest(*it);
}

void HttpRequest::OnWrite(int64_t offset, void const * buffer, size_t size)
{
#ifdef DEBUG
  static threads::ThreadID id = threads::GetCurrentThreadID();
  ASSERT_EQUAL(id, threads::GetCurrentThreadID(), ("OnWrite called from different threads"));
#endif
  m_writer.Seek(offset);
  m_writer.Write(buffer, size);
  m_progress.first += size;
  if (m_onProgress)
    m_onProgress(*this);
}

void HttpRequest::OnFinish(long httpCode, int64_t begRange, int64_t endRange)
{
  m_status = (httpCode == 200) ? ECompleted : EFailed;
  ASSERT(m_onFinish, ());
  m_onFinish(*this);
}

HttpRequest * HttpRequest::Get(string const & url, Writer & writer, CallbackT onFinish, CallbackT onProgress)
{
  HttpRequest * self = new HttpRequest(writer, onFinish, onProgress);
  self->m_threads.push_back(CreateNativeHttpRequest(url, *self));
  return self;
}

HttpRequest * HttpRequest::Post(string const & url, Writer & writer, string const & postData,
                                CallbackT onFinish, CallbackT onProgress)
{
  HttpRequest * self = new HttpRequest(writer, onFinish, onProgress);
  self->m_threads.push_back(CreateNativeHttpRequest(url, *self, 0, -1, postData));
  return self;
}

}
