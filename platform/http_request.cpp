#include "http_request.hpp"
#include "chunks_download_strategy.hpp"
#include "http_thread_callback.hpp"

#ifdef DEBUG
  #include "../base/thread.hpp"
#endif

#include "../coding/file_writer.hpp"

#include "../std/scoped_ptr.hpp"

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
HttpRequest::HttpRequest(CallbackT onFinish, CallbackT onProgress)
  : m_status(EInProgress), m_progress(make_pair(0, -1)),
    m_onFinish(onFinish), m_onProgress(onProgress)
{
}

HttpRequest::~HttpRequest()
{
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
class MemoryHttpRequest : public HttpRequest, public IHttpThreadCallback
{
  HttpThread * m_thread;

  string m_downloadedData;
  MemWriter<string> m_writer;

  virtual void OnWrite(int64_t, void const * buffer, size_t size)
  {
    m_writer.Write(buffer, size);
    m_progress.first += size;
    if (m_onProgress)
      m_onProgress(*this);
  }

  virtual void OnFinish(long httpCode, int64_t, int64_t)
  {
    m_status = (httpCode == 200) ? ECompleted : EFailed;
    m_onFinish(*this);
  }

public:
  MemoryHttpRequest(string const & url, CallbackT onFinish, CallbackT onProgress)
    : HttpRequest(onFinish, onProgress), m_writer(m_downloadedData)
  {
    m_thread = CreateNativeHttpThread(url, *this);
  }

  MemoryHttpRequest(string const & url, string const & postData,
                    CallbackT onFinish, CallbackT onProgress)
    : HttpRequest(onFinish, onProgress), m_writer(m_downloadedData)
  {
    m_thread = CreateNativeHttpThread(url, *this, 0, -1, -1, postData);
  }

  virtual ~MemoryHttpRequest()
  {
    DeleteNativeHttpThread(m_thread);
  }

  virtual string const & Data() const
  {
    return m_downloadedData;
  }
};

HttpRequest * HttpRequest::Get(string const & url, CallbackT onFinish, CallbackT onProgress)
{
  return new MemoryHttpRequest(url, onFinish, onProgress);
}

HttpRequest * HttpRequest::PostJson(string const & url, string const & postData,
                                CallbackT onFinish, CallbackT onProgress)
{
  return new MemoryHttpRequest(url, postData, onFinish, onProgress);
}

////////////////////////////////////////////////////////////////////////////////////////////////
class FileHttpRequest : public HttpRequest, public IHttpThreadCallback
{
  ChunksDownloadStrategy m_strategy;
  typedef list<pair<HttpThread *, int64_t> > ThreadsContainerT;
  ThreadsContainerT m_threads;

  string m_filePath;
  scoped_ptr<FileWriter> m_writer;

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
  FileHttpRequest(vector<string> const & urls, string const & filePath, int64_t fileSize,
                    CallbackT onFinish, CallbackT onProgress, int64_t chunkSize)
    : HttpRequest(onFinish, onProgress), m_strategy(urls, fileSize, chunkSize),
      m_filePath(filePath), m_writer(new FileWriter(filePath, FileWriter::OP_WRITE_EXISTING))
  {
    ASSERT(!urls.empty(), ("Urls list shouldn't be empty"));
    // store expected file size for future checks
    m_progress.second = fileSize;
    StartThreads();
  }

  virtual ~FileHttpRequest()
  {
    for (ThreadsContainerT::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
      DeleteNativeHttpThread(it->first);
  }

  virtual string const & Data() const
  {
    return m_filePath;
  }
};

HttpRequest * HttpRequest::GetFile(vector<string> const & urls, string const & filePath, int64_t fileSize,
                               CallbackT onFinish, CallbackT onProgress, int64_t chunkSize)
{
  return new FileHttpRequest(urls, filePath, fileSize, onFinish, onProgress, chunkSize);
}

} // namespace downloader
