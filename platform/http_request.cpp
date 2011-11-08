#include "http_request.hpp"
#include "chunks_download_strategy.hpp"
#include "http_thread_callback.hpp"

#ifdef DEBUG
  #include "../base/thread.hpp"
#endif

#include "../coding/file_writer.hpp"

class HttpThread;

namespace downloader
{

/// @return 0 if creation failed
HttpThread * CreateNativeHttpThread(string const & url,
                                          IHttpThreadCallback & callback,
                                          int64_t begRange = 0,
                                          int64_t endRange = -1,
                                          int64_t expectedSize = -1,
                                          string const & postBody = string());
void DeleteNativeHttpThread(HttpThread * request);

//////////////////////////////////////////////////////////////////////////////////////////
HttpRequest::HttpRequest(string const & filePath, CallbackT onFinish, CallbackT onProgress)
  : m_status(EInProgress), m_progress(make_pair(0, -1)),
    m_onFinish(onFinish), m_onProgress(onProgress)
{
  if (filePath.empty())
    m_writer.reset(new MemWriter<string>(m_data));
  else
  {
    m_data = filePath;
    m_writer.reset(new FileWriter(filePath, FileWriter::OP_WRITE_EXISTING));
  }
}

HttpRequest::~HttpRequest()
{
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
class SimpleHttpRequest : public HttpRequest, public IHttpThreadCallback
{
  HttpThread * m_thread;

  virtual void OnWrite(int64_t, void const * buffer, size_t size)
  {
    m_writer->Write(buffer, size);
    m_progress.first += size;
    if (m_onProgress)
      m_onProgress(*this);
  }

  virtual void OnFinish(long httpCode, int64_t, int64_t)
  {
    m_status = (httpCode == 200) ? ECompleted : EFailed;
    m_writer.reset();
    m_onFinish(*this);
  }

public:
  SimpleHttpRequest(string const & url, string const & filePath, CallbackT onFinish, CallbackT onProgress)
    : HttpRequest(filePath, onFinish, onProgress)
  {
    m_thread = CreateNativeHttpThread(url, *this);
  }

  SimpleHttpRequest(string const & url, string const & filePath, string const & postData,
                    CallbackT onFinish, CallbackT onProgress)
    : HttpRequest(filePath, onFinish, onProgress)
  {
    m_thread = CreateNativeHttpThread(url, *this, 0, -1, -1, postData);
  }

  virtual ~SimpleHttpRequest()
  {
    DeleteNativeHttpThread(m_thread);
  }
};

HttpRequest * HttpRequest::Get(string const & url, string const & filePath,
                               CallbackT onFinish, CallbackT onProgress)
{
  return new SimpleHttpRequest(url, filePath, onFinish, onProgress);
}

HttpRequest * HttpRequest::Post(string const & url, string const & filePath, string const & postData,
                                CallbackT onFinish, CallbackT onProgress)
{
  return new SimpleHttpRequest(url, filePath, postData, onFinish, onProgress);
}


////////////////////////////////////////////////////////////////////////////////////////////////
class ChunksHttpRequest : public HttpRequest, public IHttpThreadCallback
{
  ChunksDownloadStrategy m_strategy;
  typedef list<pair<HttpThread *, int64_t> > ThreadsContainerT;
  ThreadsContainerT m_threads;

  ChunksDownloadStrategy::ResultT StartThreads()
  {
    string url;
    int64_t beg, end;
    ChunksDownloadStrategy::ResultT result;
    while ((result = m_strategy.NextChunk(url, beg, end)) == ChunksDownloadStrategy::ENextChunk)
      m_threads.push_back(make_pair(CreateNativeHttpThread(url, *this, beg, end, m_progress.second), beg));
    return result;
  }

  void RemoveHttpThreadByKey(int64_t begRange)
  {
    for (ThreadsContainerT::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
      if (it->second == begRange)
      {
        DeleteNativeHttpThread(it->first);
        m_threads.erase(it);
        return;
      }
    ASSERT(false, ("Tried to remove invalid thread?"));
  }

  virtual void OnWrite(int64_t offset, void const * buffer, size_t size)
  {
  #ifdef DEBUG
    static threads::ThreadID const id = threads::GetCurrentThreadID();
    ASSERT_EQUAL(id, threads::GetCurrentThreadID(), ("OnWrite called from different threads"));
  #endif
    m_writer->Seek(offset);
    m_writer->Write(buffer, size);
  }

  virtual void OnFinish(long httpCode, int64_t begRange, int64_t endRange)
  {
#ifdef DEBUG
    static threads::ThreadID const id = threads::GetCurrentThreadID();
    ASSERT_EQUAL(id, threads::GetCurrentThreadID(), ("OnFinish called from different threads"));
#endif
    m_strategy.ChunkFinished(httpCode == 200, begRange, endRange);

    // remove completed chunk from the list, beg is the key
    RemoveHttpThreadByKey(begRange);

    ChunksDownloadStrategy::ResultT const result = StartThreads();
    // report progress
    if (result == ChunksDownloadStrategy::EDownloadSucceeded
        || result == ChunksDownloadStrategy::ENoFreeServers)
    {
      m_progress.first += m_strategy.ChunkSize();
      if (m_onProgress)
        m_onProgress(*this);
    }

    if (result == ChunksDownloadStrategy::EDownloadFailed)
      m_status = EFailed;
    else if (result == ChunksDownloadStrategy::EDownloadSucceeded)
      m_status = ECompleted;

    if (m_status != EInProgress)
    {
      m_writer.reset();
      m_onFinish(*this);
    }
  }

public:
  ChunksHttpRequest(vector<string> const & urls, string const & filePath, int64_t fileSize,
                    CallbackT onFinish, CallbackT onProgress, int64_t chunkSize)
    : HttpRequest(filePath, onFinish, onProgress), m_strategy(urls, fileSize, chunkSize)
  {
    ASSERT(!urls.empty(), ("Urls list shouldn't be empty"));
    // store expected file size for future checks
    m_progress.second = fileSize;
    StartThreads();
  }

  virtual ~ChunksHttpRequest()
  {
    for (ThreadsContainerT::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
      DeleteNativeHttpThread(it->first);
  }
};

HttpRequest * HttpRequest::GetChunks(vector<string> const & urls, string const & filePath, int64_t fileSize,
                               CallbackT onFinish, CallbackT onProgress, int64_t chunkSize)
{
  return new ChunksHttpRequest(urls, filePath, fileSize, onFinish, onProgress, chunkSize);
}

} // namespace downloader
