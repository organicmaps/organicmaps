#include "http_request.hpp"
#include "chunks_download_strategy.hpp"

#ifdef DEBUG
  #include "../base/thread.hpp"
  #include "../base/logging.hpp"
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

//////////////////////////////////////////////////////////////////////////////////////////
HttpRequest::HttpRequest(Writer & writer, CallbackT onFinish, CallbackT onProgress)
  : m_status(EInProgress), m_progress(make_pair(0, -1)), m_writer(writer),
    m_onFinish(onFinish), m_onProgress(onProgress)
{
}

HttpRequest::~HttpRequest()
{
  for (ThreadsContainerT::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
     DeleteNativeHttpRequest(*it);
}

void HttpRequest::OnSizeKnown(int64_t projectedSize)
{
  LOG(LDEBUG, ("Projected size", projectedSize));
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////

class ChunksHttpRequest : public HttpRequest
{
  ChunksDownloadStrategy m_strategy;

  ChunksDownloadStrategy::ResultT StartThreads()
  {
    string url;
    int64_t beg, end;
    ChunksDownloadStrategy::ResultT result;
    while ((result = m_strategy.NextChunk(url, beg, end)) == ChunksDownloadStrategy::ENextChunk)
      m_threads.push_back(CreateNativeHttpRequest(url, *this, beg, end));
    return result;
  }

public:
  ChunksHttpRequest(vector<string> const & urls, Writer & writer, int64_t fileSize,
                    CallbackT onFinish, CallbackT onProgress, int64_t chunkSize)
    : HttpRequest(writer, onFinish, onProgress), m_strategy(urls, fileSize, chunkSize)
  {
    ASSERT(!urls.empty(), ("Urls list shouldn't be empty"));
    StartThreads();
  }

protected:
  virtual void OnFinish(long httpCode, int64_t begRange, int64_t endRange)
  {
    m_strategy.ChunkFinished(httpCode == 200, begRange, endRange);
    ChunksDownloadStrategy::ResultT const result = StartThreads();
    if (result != ChunksDownloadStrategy::ENoFreeServers)
      HttpRequest::OnFinish(result == ChunksDownloadStrategy::EDownloadSucceeded ? 200 : -2,
                            0, -1);
  }
};

HttpRequest * HttpRequest::GetChunks(vector<string> const & urls, Writer & writer, int64_t fileSize,
                               CallbackT onFinish, CallbackT onProgress, int64_t chunkSize)
{
  return new ChunksHttpRequest(urls, writer, fileSize, onFinish, onProgress, chunkSize);
}

} // namespace downloader
