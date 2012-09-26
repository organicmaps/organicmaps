#include "http_request.hpp"
#include "chunks_download_strategy.hpp"
#include "http_thread_callback.hpp"

#include "../defines.hpp"

#ifdef DEBUG
#include "../base/thread.hpp"
#endif

#include "../coding/internal/file_data.hpp"
#include "../coding/file_writer.hpp"

#include "../base/logging.hpp"

#include "../std/scoped_ptr.hpp"


#ifdef OMIM_OS_IPHONE
#include <sys/xattr.h>
#endif

void DisableBackupForFile(string const & filePath)
{
#ifdef OMIM_OS_IPHONE
  // We need to disable iCloud backup for downloaded files.
  // This is the reason for rejecting from the AppStore

  static char const * attrName = "com.apple.MobileBackup";
  u_int8_t attrValue = 1;
  const int result = setxattr(filePath.c_str(), attrName, &attrValue, sizeof(attrValue), 0, 0);
  if (result != 0)
    LOG(LWARNING, ("Error while disabling iCloud backup for file", filePath));
#endif
}


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
void DeleteNativeHttpThread(HttpThread * thread);

//////////////////////////////////////////////////////////////////////////////////////////
/// Stores server response into the memory
class MemoryHttpRequest : public HttpRequest, public IHttpThreadCallback
{
  HttpThread * m_thread;

  string m_downloadedData;
  MemWriter<string> m_writer;

  virtual bool OnWrite(int64_t, void const * buffer, size_t size)
  {
    m_writer.Write(buffer, size);
    m_progress.first += size;
    if (m_onProgress)
      m_onProgress(*this);
    return true;
  }

  virtual void OnFinish(long httpCode, int64_t, int64_t)
  {
    if (httpCode == 200)
      m_status = ECompleted;
    else
    {
      LOG(LWARNING, ("HttpRequest error:", httpCode));
      m_status = EFailed;
    }

    m_onFinish(*this);
  }

public:
  MemoryHttpRequest(string const & url, CallbackT const & onFinish, CallbackT const & onProgress)
    : HttpRequest(onFinish, onProgress), m_writer(m_downloadedData)
  {
    m_thread = CreateNativeHttpThread(url, *this);
    ASSERT ( m_thread, () );
  }

  MemoryHttpRequest(string const & url, string const & postData,
                    CallbackT onFinish, CallbackT onProgress)
    : HttpRequest(onFinish, onProgress), m_writer(m_downloadedData)
  {
    m_thread = CreateNativeHttpThread(url, *this, 0, -1, -1, postData);
    ASSERT ( m_thread, () );
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

////////////////////////////////////////////////////////////////////////////////////////////////
class FileHttpRequest : public HttpRequest, public IHttpThreadCallback
{
  ChunksDownloadStrategy m_strategy;
  typedef list<pair<HttpThread *, int64_t> > ThreadsContainerT;
  ThreadsContainerT m_threads;

  string m_filePath;
  scoped_ptr<FileWriter> m_writer;

  size_t m_goodChunksCount;
  bool m_doCleanProgressFiles;

  ChunksDownloadStrategy::ResultT StartThreads()
  {
    string url;
    pair<int64_t, int64_t> range;
    ChunksDownloadStrategy::ResultT result;
    while ((result = m_strategy.NextChunk(url, range)) == ChunksDownloadStrategy::ENextChunk)
    {
      HttpThread * p = CreateNativeHttpThread(url, *this, range.first, range.second, m_progress.second);
      ASSERT ( p, () );
      m_threads.push_back(make_pair(p, range.first));
    }
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
    ASSERT ( false, ("Tried to remove invalid thread?") );
  }

  virtual bool OnWrite(int64_t offset, void const * buffer, size_t size)
  {
#ifdef DEBUG
    static threads::ThreadID const id = threads::GetCurrentThreadID();
    ASSERT_EQUAL(id, threads::GetCurrentThreadID(), ("OnWrite called from different threads"));
#endif

    try
    {
      m_writer->Seek(offset);
      m_writer->Write(buffer, size);
      return true;
    }
    catch (Writer::Exception const & ex)
    {
      LOG(LWARNING, ("Can't write buffer for size = ", size));
      return false;
    }
  }

  void SaveResumeChunks()
  {
    m_strategy.SaveChunks(m_progress.second, m_filePath + RESUME_FILE_EXTENSION);
  }

  /// Called for each chunk by one main (GUI) thread.
  virtual void OnFinish(long httpCode, int64_t begRange, int64_t endRange)
  {
#ifdef DEBUG
    static threads::ThreadID const id = threads::GetCurrentThreadID();
    ASSERT_EQUAL(id, threads::GetCurrentThreadID(), ("OnFinish called from different threads"));
#endif

    bool const isChunkOk = (httpCode == 200);
    m_strategy.ChunkFinished(isChunkOk, make_pair(begRange, endRange));

    // remove completed chunk from the list, beg is the key
    RemoveHttpThreadByKey(begRange);

    // report progress
    if (isChunkOk)
    {
      m_progress.first += (endRange - begRange) + 1;
      if (m_onProgress)
        m_onProgress(*this);
    }
    else
      LOG(LWARNING, (m_filePath, "HttpRequest error:", httpCode));

    ChunksDownloadStrategy::ResultT const result = StartThreads();

    if (result == ChunksDownloadStrategy::EDownloadFailed)
      m_status = EFailed;
    else if (result == ChunksDownloadStrategy::EDownloadSucceeded)
      m_status = ECompleted;

    if (isChunkOk)
    {
      // save information for download resume
      ++m_goodChunksCount;
      if (m_status != ECompleted && m_goodChunksCount % 10 == 0)
        SaveResumeChunks();
    }

    if (m_status != EInProgress)
    {
      try
      {
        m_writer.reset();
      }
      catch (Writer::Exception const & ex)
      {
        LOG(LWARNING, ("Can't close file correctly. There is not enough space, possibly."));

        ASSERT_EQUAL ( m_status, EFailed, () );
        m_status = EFailed;
      }

      // clean up resume file with chunks range on success
      if (m_status == ECompleted)
      {
        (void)my::DeleteFileX(m_filePath + RESUME_FILE_EXTENSION);

        // Rename finished file to it's original name.
        (void)my::DeleteFileX(m_filePath);
        CHECK(my::RenameFileX(m_filePath + DOWNLOADING_FILE_EXTENSION, m_filePath), ());

        DisableBackupForFile(m_filePath);
      }
      else // or save "chunks left" otherwise
        SaveResumeChunks();

      m_onFinish(*this);
    }
  }

public:
  FileHttpRequest(vector<string> const & urls, string const & filePath, int64_t fileSize,
                  CallbackT const & onFinish, CallbackT const & onProgress,
                  int64_t chunkSize, bool doCleanProgressFiles)
    : HttpRequest(onFinish, onProgress), m_strategy(urls), m_filePath(filePath),
      m_writer(new FileWriter(filePath + DOWNLOADING_FILE_EXTENSION, FileWriter::OP_WRITE_EXISTING)),
      m_goodChunksCount(0), m_doCleanProgressFiles(doCleanProgressFiles)
  {
    ASSERT ( !urls.empty(), () );

    m_progress.first = m_strategy.LoadOrInitChunks(m_filePath + RESUME_FILE_EXTENSION,
                                                   fileSize, chunkSize);
    m_progress.second = fileSize;

#ifdef OMIM_OS_IPHONE
    m_writer->Flush();
    DisableBackupForFile(filePath + DOWNLOADING_FILE_EXTENSION);
#endif

    StartThreads();
  }

  virtual ~FileHttpRequest()
  {
    for (ThreadsContainerT::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
      DeleteNativeHttpThread(it->first);

    if (m_status == EInProgress)
    {
      // means that client canceled download process, so delete all temporary files
      m_writer.reset();

      if (m_doCleanProgressFiles)
      {
        my::DeleteFileX(m_filePath + DOWNLOADING_FILE_EXTENSION);
        my::DeleteFileX(m_filePath + RESUME_FILE_EXTENSION);
      }
    }
  }

  virtual string const & Data() const
  {
    return m_filePath;
  }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////
HttpRequest::HttpRequest(CallbackT const & onFinish, CallbackT const & onProgress)
  : m_status(EInProgress), m_progress(make_pair(0, -1)),
    m_onFinish(onFinish), m_onProgress(onProgress)
{
}

HttpRequest::~HttpRequest()
{
}

HttpRequest * HttpRequest::Get(string const & url, CallbackT const & onFinish, CallbackT const & onProgress)
{
  return new MemoryHttpRequest(url, onFinish, onProgress);
}

HttpRequest * HttpRequest::PostJson(string const & url, string const & postData,
                                    CallbackT const & onFinish, CallbackT const & onProgress)
{
  return new MemoryHttpRequest(url, postData, onFinish, onProgress);
}

HttpRequest * HttpRequest::GetFile(vector<string> const & urls, string const & filePath, int64_t fileSize,
                                   CallbackT const & onFinish, CallbackT const & onProgress,
                                   int64_t chunkSize, bool doCleanProgressFiles)
{
  return new FileHttpRequest(urls, filePath, fileSize, onFinish, onProgress, chunkSize, doCleanProgressFiles);
}

} // namespace downloader
