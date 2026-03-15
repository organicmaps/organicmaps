#include "platform/http_request.hpp"

#include "platform/chunks_download_strategy.hpp"
#include "platform/gui_thread.hpp"
#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "base/thread.hpp"

#include <atomic>
#include <list>
#include <memory>

#include "defines.hpp"

namespace downloader
{
using std::string;

namespace non_http_error_code
{
string DebugPrint(long errorCode)
{
  switch (errorCode)
  {
  case kIOException: return "IO exception";
  case kWriteException: return "Write exception";
  case kInconsistentFileSize: return "Inconsistent file size";
  case kNonHttpResponse: return "Non-http response";
  case kInvalidURL: return "Invalid URL";
  case kCancelled: return "Cancelled";
  default: return std::to_string(errorCode);
  }
}
}  // namespace non_http_error_code

using AliveFlag = std::shared_ptr<std::atomic<bool>>;

////////////////////////////////////////////////////////////////////////////////////////////////
/// Downloads file using chunked strategy, backed by HttpClient async API.
class FileHttpRequest : public HttpRequest
{
  ChunksDownloadStrategy m_strategy;

  struct ChunkInfo
  {
    platform::HttpClient::RequestHandle m_handle;
    int64_t m_begRange;
  };
  std::list<ChunkInfo> m_chunks;

  AliveFlag m_alive;
  string m_filePath;
  std::unique_ptr<FileWriter> m_writer;
  size_t m_goodChunksCount;
  bool m_doCleanProgressFiles;

#ifdef DEBUG
  std::optional<threads::ThreadID> m_callbackThreadId;
  bool IsCalledOnOriginalThread()
  {
    if (!m_callbackThreadId)
      m_callbackThreadId = threads::GetCurrentThreadID();
    return m_callbackThreadId == threads::GetCurrentThreadID();
  }
#endif

  ChunksDownloadStrategy::ResultT StartChunks()
  {
    string url;
    std::pair<int64_t, int64_t> range;
    ChunksDownloadStrategy::ResultT result;
    while ((result = m_strategy.NextChunk(url, range)) == ChunksDownloadStrategy::ENextChunk)
    {
      platform::HttpClient client(url);
      client.SetRange(range.first, range.second);
      client.LoadHeaders(true);

      auto alive = m_alive;
      int64_t const begRange = range.first;
      int64_t const endRange = range.second;
      int64_t const expectedSize = m_progress.m_bytesTotal;

      auto handle = client.RunHttpRequestAsync(
          [this, alive, begRange, endRange, expectedSize](platform::HttpClient::Result result)
      {
        if (!alive->load(std::memory_order_acquire))
          return;
        platform::GuiThread().Push([this, alive, begRange, endRange, expectedSize, result = std::move(result)]() mutable
        {
          if (!alive->load(std::memory_order_acquire))
            return;
          OnChunkFinished(std::move(result), begRange, endRange, expectedSize);
        });
      });

      m_chunks.push_back({std::move(handle), range.first});
    }
    return result;
  }

  void RemoveChunkByKey(int64_t begRange)
  {
    auto it = std::find_if(m_chunks.begin(), m_chunks.end(),
                           [begRange](ChunkInfo const & c) { return c.m_begRange == begRange; });
    if (it != m_chunks.end())
      m_chunks.erase(it);
  }

  void OnChunkFinished(platform::HttpClient::Result && result, int64_t begRange, int64_t endRange, int64_t expectedSize)
  {
    ASSERT(IsCalledOnOriginalThread(), ());

    // A ranged chunk request must get 206 Partial Content.
    // 200 OK means the server ignored the Range header and returned the full file.
    bool const isRangeRequest = !(begRange == 0 && endRange < 0);
    bool isChunkOk = result.m_success && (isRangeRequest ? result.m_errorCode == 206 : result.m_errorCode == 200);

    // Validate server file size against expected using Content-Range header.
    // Content-Range is required in a 206 response (RFC 7233) and contains the total
    // file size after the slash (e.g., "bytes 0-999/50000"). Content-Length is NOT
    // used as a fallback because for ranged responses it contains the chunk size,
    // not the total file size.
    if (isChunkOk && expectedSize > 0 && isRangeRequest)
    {
      int64_t serverSize = -1;
      auto const crIt = result.m_headers.find("content-range");
      if (crIt != result.m_headers.end())
      {
        auto const slashPos = crIt->second.find('/');
        if (slashPos != string::npos)
          (void)strings::to_int(crIt->second.substr(slashPos + 1), serverSize);
      }

      if (serverSize >= 0 && serverSize != expectedSize)
      {
        LOG(LWARNING, ("Server file size", serverSize, "!= expected", expectedSize, "for range", begRange, endRange));
        isChunkOk = false;
      }
      else if (serverSize < 0)
      {
        LOG(LWARNING, ("Server didn't send Content-Range for range", begRange, endRange));
        isChunkOk = false;
      }
    }

    // Write chunk data from memory to the download file at the correct offset.
    if (isChunkOk)
    {
      try
      {
        if (result.m_serverResponse.empty())
        {
          LOG(LWARNING, ("Empty chunk response for range", begRange, endRange));
          isChunkOk = false;
        }
        else
        {
          m_writer->Seek(begRange);
          m_writer->Write(result.m_serverResponse.data(), result.m_serverResponse.size());
        }
      }
      catch (Writer::Exception const & e)
      {
        LOG(LWARNING, ("Can't write chunk at", begRange, e.Msg()));
        isChunkOk = false;
      }
    }

    string const urlError = m_strategy.ChunkFinished(isChunkOk, {begRange, endRange});
    RemoveChunkByKey(begRange);

    if (isChunkOk)
    {
      m_progress.m_bytesDownloaded += (endRange - begRange) + 1;
      if (m_onProgress)
        m_onProgress(*this);
    }
    else
    {
      auto const message = non_http_error_code::DebugPrint(result.m_errorCode);
      LOG(LWARNING, (m_filePath, "HttpRequest error:", message));
    }

    ChunksDownloadStrategy::ResultT const nextResult = StartChunks();
    if (nextResult == ChunksDownloadStrategy::EDownloadFailed)
      m_status = result.m_errorCode == 404 ? DownloadStatus::FileNotFound : DownloadStatus::Failed;
    else if (nextResult == ChunksDownloadStrategy::EDownloadSucceeded)
      m_status = DownloadStatus::Completed;

    if (isChunkOk)
    {
      ++m_goodChunksCount;
      if (m_status != DownloadStatus::Completed && m_goodChunksCount % 10 == 0)
        SaveResumeChunks();
    }

    if (m_status == DownloadStatus::InProgress)
      return;

    // 1. Save downloaded chunks if some error occurred.
    if (m_status == DownloadStatus::Failed || m_status == DownloadStatus::FileNotFound)
      SaveResumeChunks();

    // 2. Free file handle.
    CloseWriter();

    // 3. Clean up resume file with chunks range on success.
    if (m_status == DownloadStatus::Completed)
    {
      Platform::RemoveFileIfExists(m_filePath + RESUME_FILE_EXTENSION);
      Platform::RemoveFileIfExists(m_filePath);
      base::RenameFileX(m_filePath + DOWNLOADING_FILE_EXTENSION, m_filePath);
    }

    // 4. Finish downloading.
    m_onFinish(*this);
  }

  void SaveResumeChunks()
  {
    try
    {
      m_writer->Flush();
      m_strategy.SaveChunks(m_progress.m_bytesTotal, m_filePath + RESUME_FILE_EXTENSION);
    }
    catch (Writer::Exception const & e)
    {
      LOG(LWARNING, ("Can't flush writer", e.Msg()));
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
      m_status = DownloadStatus::Failed;
    }
  }

public:
  FileHttpRequest(std::vector<string> const & urls, string const & filePath, int64_t fileSize, Callback && onFinish,
                  Callback && onProgress, int64_t chunkSize, bool doCleanProgressFiles)
    : HttpRequest(std::move(onFinish), std::move(onProgress))
    , m_strategy(urls)
    , m_alive(std::make_shared<std::atomic<bool>>(true))
    , m_filePath(filePath)
    , m_goodChunksCount(0)
    , m_doCleanProgressFiles(doCleanProgressFiles)
  {
    ASSERT(!urls.empty(), ());

    // Load resume downloading information.
    m_progress.m_bytesDownloaded = m_strategy.LoadOrInitChunks(m_filePath + RESUME_FILE_EXTENSION, fileSize, chunkSize);
    m_progress.m_bytesTotal = fileSize;

    FileWriter::Op openMode = FileWriter::OP_WRITE_TRUNCATE;
    if (m_progress.m_bytesDownloaded != 0)
    {
      uint64_t size = 0;
      // The file must exist and be large enough to cover all completed chunks.
      if (base::GetFileSize(filePath + DOWNLOADING_FILE_EXTENSION, size) && size <= static_cast<uint64_t>(fileSize) &&
          static_cast<int64_t>(size) >= m_strategy.RequiredFileSize())
      {
        openMode = FileWriter::OP_WRITE_EXISTING;
      }
      else
      {
        LOG(LWARNING, ("Restarting download: file size", size,
                       "doesn't match resume state, required >=", m_strategy.RequiredFileSize()));
        m_progress.m_bytesDownloaded = 0;
        m_strategy.InitChunks(fileSize, chunkSize);
      }
    }

    std::unique_ptr<FileWriter> writer(new FileWriter(filePath + DOWNLOADING_FILE_EXTENSION, openMode));
    m_writer.swap(writer);
    Platform::DisableBackupForFile(filePath + DOWNLOADING_FILE_EXTENSION);
    StartChunks();
  }

  ~FileHttpRequest() override
  {
    m_alive->store(false, std::memory_order_release);

    for (auto & chunk : m_chunks)
      chunk.m_handle.Cancel();
    m_chunks.clear();

    if (m_status == DownloadStatus::InProgress)
    {
      CloseWriter();
      if (m_doCleanProgressFiles)
      {
        Platform::RemoveFileIfExists(m_filePath + DOWNLOADING_FILE_EXTENSION);
        Platform::RemoveFileIfExists(m_filePath + RESUME_FILE_EXTENSION);
      }
    }
  }

  string const & GetFilePath() const override { return m_filePath; }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////
HttpRequest::HttpRequest(Callback && onFinish, Callback && onProgress)
  : m_status(DownloadStatus::InProgress)
  , m_progress(Progress::Unknown())
  , m_onFinish(std::move(onFinish))
  , m_onProgress(std::move(onProgress))
{}

HttpRequest::~HttpRequest() {}

HttpRequest * HttpRequest::GetFile(std::vector<string> const & urls, string const & filePath, int64_t fileSize,
                                   Callback && onFinish, Callback && onProgress, int64_t chunkSize,
                                   bool doCleanOnCancel)
{
  try
  {
    return new FileHttpRequest(urls, filePath, fileSize, std::move(onFinish), std::move(onProgress), chunkSize,
                               doCleanOnCancel);
  }
  catch (FileWriter::Exception const & e)
  {
    LOG(LWARNING, ("Can't create file", filePath, "with size", fileSize, e.Msg()));
  }
  return nullptr;
}
}  // namespace downloader
