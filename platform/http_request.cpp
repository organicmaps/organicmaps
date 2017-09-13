#include "platform/chunks_download_strategy.hpp"
#include "platform/http_request.hpp"
#include "platform/http_thread_callback.hpp"
#include "platform/platform.hpp"

#include "defines.hpp"

#ifdef DEBUG
#include "base/thread.hpp"
#endif

#include "coding/internal/file_data.hpp"
#include "coding/file_writer.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/list.hpp"
#include "std/unique_ptr.hpp"

#include "3party/Alohalytics/src/alohalytics.h"

#ifdef OMIM_OS_IPHONE

#include <sys/xattr.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFURL.h>
// declaration is taken from NSObjCRuntime.h to avoid including of ObjC code
extern "C" double NSFoundationVersionNumber;

#endif

void DisableBackupForFile(string const & filePath)
{
#ifdef OMIM_OS_IPHONE
  // We need to disable iCloud backup for downloaded files.
  // This is the reason for rejecting from the AppStore
  // https://developer.apple.com/library/iOS/qa/qa1719/_index.html

  // value is taken from NSObjCRuntime.h to avoid including of ObjC code
  #define NSFoundationVersionNumber_iOS_5_1  890.10
  if (NSFoundationVersionNumber >= NSFoundationVersionNumber_iOS_5_1)
  {
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault,
                                                           reinterpret_cast<unsigned char const *>(filePath.c_str()),
                                                           filePath.size(),
                                                           0);
    CFErrorRef err;
    signed char valueRaw = 1; // BOOL YES
    CFNumberRef value = CFNumberCreate(kCFAllocatorDefault, kCFNumberCharType, &valueRaw);
    if (!CFURLSetResourcePropertyForKey(url, kCFURLIsExcludedFromBackupKey, value, &err))
    {
      LOG(LWARNING, ("Error while disabling iCloud backup for file", filePath));
    }
    CFRelease(value);
    CFRelease(url);
  }
  else
  {
    static char const * attrName = "com.apple.MobileBackup";
    u_int8_t attrValue = 1;
    const int result = setxattr(filePath.c_str(), attrName, &attrValue, sizeof(attrValue), 0, 0);
    if (result != 0)
      LOG(LWARNING, ("Error while disabling iCloud backup for file", filePath));
  }
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

  string m_requestUrl;
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
      alohalytics::LogEvent(
          "$httpRequestError",
          {{"url", m_requestUrl}, {"code", strings::to_string(httpCode)}, {"servers", "1"}});
      m_status = EFailed;
    }

    m_onFinish(*this);
  }

public:
  MemoryHttpRequest(string const & url, CallbackT const & onFinish, CallbackT const & onProgress)
    : HttpRequest(onFinish, onProgress), m_requestUrl(url), m_writer(m_downloadedData)
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
  typedef pair<HttpThread *, int64_t> ThreadHandleT;
  typedef list<ThreadHandleT> ThreadsContainerT;
  ThreadsContainerT m_threads;

  string m_filePath;
  unique_ptr<FileWriter> m_writer;

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

  class ThreadByPos
  {
    int64_t m_pos;
  public:
    ThreadByPos(int64_t pos) : m_pos(pos) {}
    inline bool operator() (ThreadHandleT const & p) const
    {
      return (p.second == m_pos);
    }
  };

  void RemoveHttpThreadByKey(int64_t begRange)
  {
    ThreadsContainerT::iterator it = find_if(m_threads.begin(), m_threads.end(),
                                             ThreadByPos(begRange));
    if (it != m_threads.end())
    {
      HttpThread * p = it->first;
      m_threads.erase(it);
      DeleteNativeHttpThread(p);
    }
    else
      LOG(LERROR, ("Tried to remove invalid thread for position", begRange));
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
    catch (Writer::Exception const & e)
    {
      LOG(LWARNING, ("Can't write buffer for size", size, e.Msg()));
      return false;
    }
  }

  void SaveResumeChunks()
  {
    if (m_writer == nullptr)
      return;

    try
    {
      // Flush writer before saving downloaded chunks.
      m_writer->Flush();

      m_strategy.SaveChunks(m_progress.second, m_filePath + RESUME_FILE_EXTENSION);
    }
    catch (Writer::Exception const & e)
    {
      LOG(LWARNING, ("Can't flush writer", e.Msg()));
    }
  }

  /// Called for each chunk by one main (GUI) thread.
  virtual void OnFinish(long httpCode, int64_t begRange, int64_t endRange)
  {
#ifdef DEBUG
    static threads::ThreadID const id = threads::GetCurrentThreadID();
    ASSERT_EQUAL(id, threads::GetCurrentThreadID(), ("OnFinish called from different threads"));
#endif

    bool const isChunkOk = (httpCode == 200);
    string const urlError = m_strategy.ChunkFinished(isChunkOk, make_pair(begRange, endRange));

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
    {
      LOG(LWARNING, (m_filePath, "HttpRequest error:", httpCode));
      alohalytics::LogEvent("$httpRequestError",
                            {{"url", urlError},
                             {"code", strings::to_string(httpCode)},
                             {"servers", strings::to_string(m_strategy.ActiveServersCount())}});
    }

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
    else if (result == ChunksDownloadStrategy::ENoFreeServers)
    {
      // There is no any server which is able to re-download chunk.
      m_status = EFailed;
    }

    if (m_status != EInProgress)
    {
      // 1. Save downloaded chunks if some error occured.
      if (m_status != ECompleted)
        SaveResumeChunks();

      // 2. Free file handle.
      CloseWriter();

      // 3. Clean up resume file with chunks range on success
      if (m_status == ECompleted)
      {
        (void)my::DeleteFileX(m_filePath + RESUME_FILE_EXTENSION);

        // Rename finished file to it's original name.
        if (Platform::IsFileExistsByFullPath(m_filePath))
          (void)my::DeleteFileX(m_filePath);
        CHECK(my::RenameFileX(m_filePath + DOWNLOADING_FILE_EXTENSION, m_filePath),
              (m_filePath, strerror(errno)));

        DisableBackupForFile(m_filePath);
      }

      // 4. Finish downloading.
      m_onFinish(*this);
    }
  }

  void CloseWriter()
  {
    try
    {
      m_writer.reset();
    }
    catch (Writer::Exception const & e)
    {
      LOG(LWARNING, ("Can't close file correctly", e.Msg()));

      m_status = EFailed;
    }
  }

public:
  FileHttpRequest(vector<string> const & urls, string const & filePath, int64_t fileSize,
                  CallbackT const & onFinish, CallbackT const & onProgress,
                  int64_t chunkSize, bool doCleanProgressFiles)
    : HttpRequest(onFinish, onProgress), m_strategy(urls), m_filePath(filePath),
      m_goodChunksCount(0), m_doCleanProgressFiles(doCleanProgressFiles)
  {
    ASSERT ( !urls.empty(), () );

    // Load resume downloading information.
    m_progress.first = m_strategy.LoadOrInitChunks(m_filePath + RESUME_FILE_EXTENSION,
                                                   fileSize, chunkSize);
    m_progress.second = fileSize;

    FileWriter::Op openMode = FileWriter::OP_WRITE_TRUNCATE;
    if (m_progress.first != 0)
    {
      // Check that resume information is correct with existing file.
      uint64_t size;
      if (my::GetFileSize(filePath + DOWNLOADING_FILE_EXTENSION, size) && size <= fileSize)
        openMode = FileWriter::OP_WRITE_EXISTING;
      else
        m_strategy.InitChunks(fileSize, chunkSize);
    }

    // Create file and reserve needed size.
    unique_ptr<FileWriter> writer(new FileWriter(filePath + DOWNLOADING_FILE_EXTENSION, openMode));
    // Reserving disk space is very slow on a device.
    //writer->Reserve(fileSize);

    // Assign here, because previous functions can throw an exception.
    m_writer.swap(writer);

#ifdef OMIM_OS_IPHONE
    DisableBackupForFile(filePath + DOWNLOADING_FILE_EXTENSION);
#endif

    (void)StartThreads();
  }

  virtual ~FileHttpRequest()
  {
    // Do safe delete with removing from list in case if DeleteNativeHttpThread
    // can produce final notifications to this->OnFinish().
    while (!m_threads.empty())
    {
      HttpThread * p = m_threads.back().first;
      m_threads.pop_back();
      DeleteNativeHttpThread(p);
    }

    if (m_status == EInProgress)
    {
      // means that client canceled download process, so delete all temporary files
      CloseWriter();

      if (m_doCleanProgressFiles)
      {
        (void)my::DeleteFileX(m_filePath + DOWNLOADING_FILE_EXTENSION);
        (void)my::DeleteFileX(m_filePath + RESUME_FILE_EXTENSION);
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

HttpRequest * HttpRequest::GetFile(vector<string> const & urls,
                                   string const & filePath, int64_t fileSize,
                                   CallbackT const & onFinish, CallbackT const & onProgress,
                                   int64_t chunkSize, bool doCleanOnCancel)
{
  try
  {
    return new FileHttpRequest(urls, filePath, fileSize, onFinish, onProgress, chunkSize, doCleanOnCancel);
  }
  catch (FileWriter::Exception const & e)
  {
    // Can't create or open file for writing.
    LOG(LWARNING, ("Can't create file", filePath, "with size", fileSize, e.Msg()));
  }
  return nullptr;
}

} // namespace downloader
