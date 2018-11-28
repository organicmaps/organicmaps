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

class HttpThread;

namespace downloader
{
namespace non_http_error_code
{
string DebugPrint(long errorCode)
{
  switch (errorCode)
  {
  case kIOException:
    return "IO exception";
  case kWriteException:
    return "Write exception";
  case kInconsistentFileSize:
    return "Inconsistent file size";
  case kNonHttpResponse:
    return "Non-http response";
  case kInvalidURL:
    return "Invalid URL";
  case kCancelled:
    return "Cancelled";
  default:
    return to_string(errorCode);
  }
}
}  // namespace non_http_error_code

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

  virtual void OnFinish(long httpOrErrorCode, int64_t, int64_t)
  {
    if (httpOrErrorCode == 200)
      m_status = Status::Completed;
    else
    {
      auto const message = non_http_error_code::DebugPrint(httpOrErrorCode);
      LOG(LWARNING, ("HttpRequest error:", message));
      alohalytics::LogEvent(
          "$httpRequestError",
          {{"url", m_requestUrl}, {"code", message}, {"servers", "1"}});
      if (httpOrErrorCode == 404)
        m_status = Status::FileNotFound;
      else
        m_status = Status::Failed;
    }

    m_onFinish(*this);
  }

public:
  MemoryHttpRequest(string const & url, Callback const & onFinish, Callback const & onProgress)
    : HttpRequest(onFinish, onProgress), m_requestUrl(url), m_writer(m_downloadedData)
  {
    m_thread = CreateNativeHttpThread(url, *this);
    ASSERT ( m_thread, () );
  }

  MemoryHttpRequest(string const & url, string const & postData,
                    Callback onFinish, Callback onProgress)
    : HttpRequest(onFinish, onProgress), m_writer(m_downloadedData)
  {
    m_thread = CreateNativeHttpThread(url, *this, 0, -1, -1, postData);
    ASSERT ( m_thread, () );
  }

  virtual ~MemoryHttpRequest()
  {
    DeleteNativeHttpThread(m_thread);
  }

  virtual string const & GetData() const
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
  virtual void OnFinish(long httpOrErrorCode, int64_t begRange, int64_t endRange)
  {
#ifdef DEBUG
    static threads::ThreadID const id = threads::GetCurrentThreadID();
    ASSERT_EQUAL(id, threads::GetCurrentThreadID(), ("OnFinish called from different threads"));
#endif

    bool const isChunkOk = (httpOrErrorCode == 200);
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
      auto const message = non_http_error_code::DebugPrint(httpOrErrorCode);
      LOG(LWARNING, (m_filePath, "HttpRequest error:", message));
      alohalytics::LogEvent("$httpRequestError",
                            {{"url", urlError},
                             {"code", message},
                             {"servers", strings::to_string(m_strategy.ActiveServersCount())}});
    }

    ChunksDownloadStrategy::ResultT const result = StartThreads();
    if (result == ChunksDownloadStrategy::EDownloadFailed)
      m_status = httpOrErrorCode == 404 ? Status::FileNotFound : Status::Failed;
    else if (result == ChunksDownloadStrategy::EDownloadSucceeded)
      m_status = Status::Completed;

    if (isChunkOk)
    {
      // save information for download resume
      ++m_goodChunksCount;
      if (m_status != Status::Completed && m_goodChunksCount % 10 == 0)
        SaveResumeChunks();
    }

    if (m_status == Status::InProgress)
      return;

    // 1. Save downloaded chunks if some error occured.
    if (m_status == Status::Failed || m_status == Status::FileNotFound)
      SaveResumeChunks();

    // 2. Free file handle.
    CloseWriter();

    // 3. Clean up resume file with chunks range on success
    if (m_status == Status::Completed)
    {
      base::DeleteFileX(m_filePath + RESUME_FILE_EXTENSION);

      // Rename finished file to it's original name.
      if (Platform::IsFileExistsByFullPath(m_filePath))
        base::DeleteFileX(m_filePath);
      CHECK(base::RenameFileX(m_filePath + DOWNLOADING_FILE_EXTENSION, m_filePath),
            (m_filePath, strerror(errno)));

      Platform::DisableBackupForFile(m_filePath);
    }

    // 4. Finish downloading.
    m_onFinish(*this);
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

      m_status = Status::Failed;
    }
  }

public:
  FileHttpRequest(vector<string> const & urls, string const & filePath, int64_t fileSize,
                  Callback const & onFinish, Callback const & onProgress,
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
      if (base::GetFileSize(filePath + DOWNLOADING_FILE_EXTENSION, size) &&
              size <= static_cast<uint64_t>(fileSize))
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
    Platform::DisableBackupForFile(filePath + DOWNLOADING_FILE_EXTENSION);
    StartThreads();
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

    if (m_status == Status::InProgress)
    {
      // means that client canceled download process, so delete all temporary files
      CloseWriter();

      if (m_doCleanProgressFiles)
      {
        base::DeleteFileX(m_filePath + DOWNLOADING_FILE_EXTENSION);
        base::DeleteFileX(m_filePath + RESUME_FILE_EXTENSION);
      }
    }
  }

  virtual string const & GetData() const
  {
    return m_filePath;
  }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////
HttpRequest::HttpRequest(Callback const & onFinish, Callback const & onProgress)
  : m_status(Status::InProgress), m_progress(make_pair(0, -1)),
    m_onFinish(onFinish), m_onProgress(onProgress)
{
}

HttpRequest::~HttpRequest()
{
}

HttpRequest * HttpRequest::Get(string const & url, Callback const & onFinish, Callback const & onProgress)
{
  return new MemoryHttpRequest(url, onFinish, onProgress);
}

HttpRequest * HttpRequest::PostJson(string const & url, string const & postData,
                                    Callback const & onFinish, Callback const & onProgress)
{
  return new MemoryHttpRequest(url, postData, onFinish, onProgress);
}

HttpRequest * HttpRequest::GetFile(vector<string> const & urls,
                                   string const & filePath, int64_t fileSize,
                                   Callback const & onFinish, Callback const & onProgress,
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

string DebugPrint(HttpRequest::Status status)
{
  switch (status)
  {
  case HttpRequest::Status::InProgress:
    return "In progress";
  case HttpRequest::Status::Completed:
    return "Completed";
  case HttpRequest::Status::Failed:
    return "Failed";
  case HttpRequest::Status::FileNotFound:
    return "File not found";
  }
  UNREACHABLE();
}
} // namespace downloader
