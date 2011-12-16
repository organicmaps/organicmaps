#include "http_request.hpp"
#include "chunks_download_strategy.hpp"
#include "http_thread_callback.hpp"

#include "../defines.hpp"

#ifdef DEBUG
  #include "../base/thread.hpp"
#endif

#include "../base/std_serialization.hpp"
#include "../base/logging.hpp"

#include "../coding/file_writer_stream.hpp"
#include "../coding/file_reader_stream.hpp"

#include "../std/scoped_ptr.hpp"

#ifdef OMIM_OS_IPHONE

#include <sys/xattr.h>

void DisableiCloudBackupForFile(string const & filePath)
{
  static char const * attrName = "com.apple.MobileBackup";
  u_int8_t attrValue = 1;
  const int result = setxattr(filePath.c_str(), attrName, &attrValue, sizeof(attrValue), 0, 0);
  if (result != 0)
    LOG(LWARNING, ("Error while disabling iCloud backup for file", filePath));
}

#endif // OMIM_OS_IPHONE

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

////////////////////////////////////////////////////////////////////////////////////////////////
class FileHttpRequest : public HttpRequest, public IHttpThreadCallback
{
  ChunksDownloadStrategy m_strategy;
  typedef list<pair<HttpThread *, int64_t> > ThreadsContainerT;
  ThreadsContainerT m_threads;

  string m_filePath;
  scoped_ptr<FileWriter> m_writer;

  size_t m_goodChunksCount;

  ChunksDownloadStrategy::ResultT StartThreads()
  {
    string url;
    ChunksDownloadStrategy::RangeT range;
    ChunksDownloadStrategy::ResultT result;
    while ((result = m_strategy.NextChunk(url, range)) == ChunksDownloadStrategy::ENextChunk)
      m_threads.push_back(make_pair(CreateNativeHttpThread(url, *this, range.first, range.second,
                                                           m_progress.second), range.first));
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

  /// Called by each http thread
  virtual void OnFinish(long httpCode, int64_t begRange, int64_t endRange)
  {
#ifdef DEBUG
    static threads::ThreadID const id = threads::GetCurrentThreadID();
    ASSERT_EQUAL(id, threads::GetCurrentThreadID(), ("OnFinish called from different threads"));
#endif
    bool const isChunkOk = (httpCode == 200);
    m_strategy.ChunkFinished(isChunkOk, ChunksDownloadStrategy::RangeT(begRange, endRange));

    // remove completed chunk from the list, beg is the key
    RemoveHttpThreadByKey(begRange);

    // report progress
    if (isChunkOk)
    {
      m_progress.first += (endRange - begRange) + 1;
      if (m_onProgress)
        m_onProgress(*this);
    }

    ChunksDownloadStrategy::ResultT const result = StartThreads();

    if (result == ChunksDownloadStrategy::EDownloadFailed)
      m_status = EFailed;
    else if (result == ChunksDownloadStrategy::EDownloadSucceeded)
      m_status = ECompleted;

    if (isChunkOk)
    { // save information for download resume
      ++m_goodChunksCount;
      if (m_status != ECompleted && m_goodChunksCount % 10 == 0)
        SaveRanges(m_filePath + RESUME_FILE_EXTENSION, m_strategy.ChunksLeft());
    }

    if (m_status != EInProgress)
    {
      m_writer.reset();
      // clean up resume file with chunks range on success
      if (m_strategy.ChunksLeft().empty())
      {
        FileWriter::DeleteFileX(m_filePath + RESUME_FILE_EXTENSION);
        // rename finished file to it's original name
        rename((m_filePath + DOWNLOADING_FILE_EXTENSION).c_str(), m_filePath.c_str());
#ifdef OMIM_OS_IPHONE
        // We need to disable iCloud backup for downloaded files.
        // This is the reason for rejecting from the AppStore
        DisableiCloudBackupForFile(m_filePath.c_str());
#endif
      }
      else // or save "chunks left" otherwise
        SaveRanges(m_filePath + RESUME_FILE_EXTENSION, m_strategy.ChunksLeft());
      m_onFinish(*this);
    }
  }

  /// @return true if ranges are present and loaded
  static bool LoadRanges(string const & file, ChunksDownloadStrategy::RangesContainerT & ranges)
  {
    ranges.clear();
    try
    {
      FileReaderStream frs(file);
      frs >> ranges;
    }
    catch (std::exception const &)
    {
      return false;
    }
    return !ranges.empty();
  }

  static void SaveRanges(string const & file, ChunksDownloadStrategy::RangesContainerT const & ranges)
  {
    // Delete resume file if ranges are empty
    if (ranges.empty())
      FileWriter::DeleteFileX(file);
    else
    {
      FileWriterStream fws(file);
      fws << ranges;
    }
#ifdef OMIM_OS_IPHONE
    DisableiCloudBackupForFile(file);
#endif
  }

  struct CalcRanges
  {
    int64_t & m_summ;
    CalcRanges(int64_t & summ) : m_summ(summ) {}
    void operator()(ChunksDownloadStrategy::RangeT const & range)
    {
      m_summ += (range.second - range.first) + 1;
    }
  };

public:
  FileHttpRequest(vector<string> const & urls, string const & filePath, int64_t fileSize,
                    CallbackT onFinish, CallbackT onProgress, int64_t chunkSize)
    : HttpRequest(onFinish, onProgress), m_strategy(urls, fileSize, chunkSize),
      m_filePath(filePath),
      m_writer(new FileWriter(filePath + DOWNLOADING_FILE_EXTENSION, FileWriter::OP_WRITE_EXISTING)),
      m_goodChunksCount(0)
  {
    ASSERT_GREATER(fileSize, 0, ("At the moment only known file sizes are supported"));
    ASSERT(!urls.empty(), ("Urls list shouldn't be empty"));
    m_progress.second = fileSize;

    // Resume support - load chunks which should be downloaded (if they're present)
    ChunksDownloadStrategy::RangesContainerT ranges;
    if (LoadRanges(filePath + RESUME_FILE_EXTENSION, ranges))
    {
      // fix progress
      int64_t sizeLeft = 0;
      for_each(ranges.begin(), ranges.end(), CalcRanges(sizeLeft));
      m_progress.first = fileSize - sizeLeft;
      m_strategy.SetChunksToDownload(ranges);
    }

#ifdef OMIM_OS_IPHONE
    m_writer->Flush();
    DisableiCloudBackupForFile(filePath + DOWNLOADING_FILE_EXTENSION);
#endif
    StartThreads();
  }

  virtual ~FileHttpRequest()
  {
    for (ThreadsContainerT::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
      DeleteNativeHttpThread(it->first);

    if (m_status == EInProgress)
    { // means that client canceled donwload process
      // so delete all temporary files
      m_writer.reset();
      FileWriter::DeleteFileX(m_filePath + DOWNLOADING_FILE_EXTENSION);
      FileWriter::DeleteFileX(m_filePath + RESUME_FILE_EXTENSION);
    }
  }

  virtual string const & Data() const
  {
    return m_filePath;
  }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////
HttpRequest::HttpRequest(CallbackT onFinish, CallbackT onProgress)
  : m_status(EInProgress), m_progress(make_pair(0, -1)),
    m_onFinish(onFinish), m_onProgress(onProgress)
{
}

HttpRequest::~HttpRequest()
{
}

HttpRequest * HttpRequest::Get(string const & url, CallbackT onFinish, CallbackT onProgress)
{
  return new MemoryHttpRequest(url, onFinish, onProgress);
}

HttpRequest * HttpRequest::PostJson(string const & url, string const & postData,
                                CallbackT onFinish, CallbackT onProgress)
{
  return new MemoryHttpRequest(url, postData, onFinish, onProgress);
}

HttpRequest * HttpRequest::GetFile(vector<string> const & urls, string const & filePath, int64_t fileSize,
                               CallbackT onFinish, CallbackT onProgress, int64_t chunkSize)
{
  return new FileHttpRequest(urls, filePath, fileSize, onFinish, onProgress, chunkSize);
}

} // namespace downloader
