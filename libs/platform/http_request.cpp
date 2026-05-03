#include "platform/http_request.hpp"

#include "platform/chunks_download_strategy.hpp"
#include "platform/gui_thread.hpp"
#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include "base/logging.hpp"
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
/// Each chunk is streamed directly to disk via SetReceivedFileSegment — the chunk body
/// is never materialized in RAM on the transport side, so Android's Large Object Space
/// is not churned by per-chunk 512 KB byte[] allocations.
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
  string m_downloadingPath;  // m_filePath + DOWNLOADING_FILE_EXTENSION, cached.
  size_t m_goodChunksCount;
  bool m_doCleanOnCancel;

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
    ChunksDownloadStrategy::ResultT result = ChunksDownloadStrategy::ENoFreeServers;
    while ((result = m_strategy.NextChunk(url, range)) == ChunksDownloadStrategy::ENextChunk)
    {
      int64_t const begRange = range.first;
      int64_t const endRange = range.second;

      platform::HttpClient client(url);
      client.SetRange(begRange, endRange);
      // Stream the chunk body directly into the .downloading file at begRange. The
      // transport validates 206 + Content-Range against the requested segment before
      // writing any byte, so no corruption if a mirror returns 200 + full body instead
      // of a partial response.
      client.SetReceivedFileSegment({m_downloadingPath, begRange, endRange - begRange + 1, m_progress.m_bytesTotal});
      client.LoadHeaders(true);

      auto alive = m_alive;
      auto handle = client.RunHttpRequestAsync([this, alive, begRange, endRange](platform::HttpClient::Result result)
      {
        if (!alive->load(std::memory_order_acquire))
          return;
        platform::GuiThread().Push([this, alive, begRange, endRange, result = std::move(result)]() mutable
        {
          if (!alive->load(std::memory_order_acquire))
            return;
          OnChunkFinished(std::move(result), begRange, endRange);
        });
      });

      m_chunks.push_back({std::move(handle), begRange});
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

  void OnChunkFinished(platform::HttpClient::Result && result, int64_t begRange, int64_t endRange)
  {
    ASSERT(IsCalledOnOriginalThread(), ());

    // The transport (SetReceivedFileSegment) has already validated 206 + Content-Range
    // and streamed the body to disk. Protocol violations surface as kInconsistentFileSize.
    bool const isRangeRequest = !(begRange == 0 && endRange < 0);
    bool const isChunkOk = result.m_success && (isRangeRequest ? result.m_errorCode == 206 : result.m_errorCode == 200);

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
      if (m_status != DownloadStatus::Completed && m_goodChunksCount % kPeriodicResumeSaveInterval == 0)
        SaveResumeChunks();
    }

    if (m_status == DownloadStatus::InProgress)
      return;

    // 1. Save downloaded chunks if some error occurred.
    if (m_status == DownloadStatus::Failed || m_status == DownloadStatus::FileNotFound)
      SaveResumeChunks();

    // 2. Clean up resume file with chunks range on success.
    if (m_status == DownloadStatus::Completed)
    {
      Platform::RemoveFileIfExists(m_filePath + RESUME_FILE_EXTENSION);
      Platform::RemoveFileIfExists(m_filePath);
      base::RenameFileX(m_downloadingPath, m_filePath);
    }

    // 3. Finish downloading.
    m_onFinish(*this);
  }

  void SaveResumeChunks()
  {
    // No writer flush needed — each chunk's transport closes its own file handle
    // before the completion callback fires. Completed chunks are durable on disk
    // (modulo kernel buffer flushing, same as the historical FileWriter::Flush() path).
    m_strategy.SaveChunks(m_progress.m_bytesTotal, m_filePath + RESUME_FILE_EXTENSION);
  }

public:
  FileHttpRequest(std::vector<string> const & urls, string const & filePath, int64_t fileSize, Callback && onFinish,
                  Callback && onProgress, int64_t chunkSize, bool doCleanOnCancel)
    : HttpRequest(std::move(onFinish), std::move(onProgress))
    , m_strategy(urls)
    , m_alive(std::make_shared<std::atomic<bool>>(true))
    , m_filePath(filePath)
    , m_downloadingPath(filePath + DOWNLOADING_FILE_EXTENSION)
    , m_goodChunksCount(0)
    , m_doCleanOnCancel(doCleanOnCancel)
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
      if (base::GetFileSize(m_downloadingPath, size) && size <= static_cast<uint64_t>(fileSize) &&
          static_cast<int64_t>(size) >= m_strategy.RequiredFileSize())
      {
        openMode = FileWriter::OP_WRITE_EXISTING;
        LOG(LINFO, ("Resuming download of", m_filePath, "from", m_progress.m_bytesDownloaded, "of", fileSize, "bytes"));
      }
      else
      {
        LOG(LWARNING, ("Restarting download: file size", size,
                       "doesn't match resume state, required >=", m_strategy.RequiredFileSize()));
        m_progress.m_bytesDownloaded = 0;
        m_strategy.InitChunks(fileSize, chunkSize);
      }
    }

    // Create/validate the .downloading file, then close it immediately. Each chunk's
    // transport opens its own random-access handle at the chunk's begin offset.
    {
      FileWriter writer(m_downloadingPath, openMode);
    }
    Platform::DisableBackupForFile(m_downloadingPath);
    StartChunks();
  }

  ~FileHttpRequest() override
  {
    m_alive->store(false, std::memory_order_release);

    for (auto & chunk : m_chunks)
      chunk.m_handle.Cancel();

    if (m_status == DownloadStatus::InProgress)
    {
      if (m_doCleanOnCancel)
      {
        // Delete temp files synchronously. Any still-in-flight transport writes will land
        // on the unlinked inode (POSIX: data goes to limbo, inode reclaimed when last fd
        // closes) or fail silently (Windows) — either way, no interaction with a potential
        // new download that reuses the same path. A deferred-cleanup scheme here would
        // introduce a cancel+retry race where the cleanup deletes the NEW download's file.
        Platform::RemoveFileIfExists(m_downloadingPath);
        Platform::RemoveFileIfExists(m_filePath + RESUME_FILE_EXTENSION);
      }
      else
      {
        // Periodic SaveResumeChunks() runs only every kPeriodicResumeSaveInterval-th completed
        // chunk, so .resume may lag the .downloading file by up to N-1 chunks. Flush now so
        // those chunks aren't re-downloaded on next launch.
        LOG(LINFO, ("Preserving partial download for", m_filePath, "at", m_progress.m_bytesDownloaded, "of",
                    m_progress.m_bytesTotal, "bytes"));
        SaveResumeChunks();
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
                                   Callback && onFinish, Callback && onProgress, bool doCleanOnCancel,
                                   int64_t chunkSize)
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
