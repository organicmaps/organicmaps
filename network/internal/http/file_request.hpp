#pragma once

#include "platform/platform.hpp"

#include "coding/file_writer.hpp"

#include "network/chunks_download_strategy.hpp"
#include "network/http/request.hpp"
#include "network/http/thread.hpp"
#include "network/non_http_error_code.hpp"

namespace om::network::http::internal
{
class FileRequest : public Request, public IThreadCallback
{
  using ThreadHandleT = std::pair<Thread *, int64_t>;
  using ThreadsContainerT = std::list<ThreadHandleT>;

  class ThreadByPos
  {
    int64_t m_pos;

  public:
    explicit ThreadByPos(int64_t pos) : m_pos(pos) {}
    inline bool operator()(ThreadHandleT const & p) const { return (p.second == m_pos); }
  };

public:
  FileRequest(std::vector<std::string> const & urls, std::string const & filePath, int64_t fileSize,
              Callback && onFinish, Callback && onProgress, int64_t chunkSize, bool doCleanProgressFiles)
    : Request(std::move(onFinish), std::move(onProgress))
    , m_strategy(urls)
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
      // Check that resume information is correct with existing file.
      uint64_t size;
      if (base::GetFileSize(filePath + DOWNLOADING_FILE_EXTENSION, size) && size <= static_cast<uint64_t>(fileSize))
        openMode = FileWriter::OP_WRITE_EXISTING;
      else
        m_strategy.InitChunks(fileSize, chunkSize);
    }

    // Create file and reserve needed size.
    std::unique_ptr<FileWriter> writer(new FileWriter(filePath + DOWNLOADING_FILE_EXTENSION, openMode));

    // Assign here, because previous functions can throw an exception.
    m_writer.swap(writer);
    Platform::DisableBackupForFile(filePath + DOWNLOADING_FILE_EXTENSION);
    StartThreads();
  }

  ~FileRequest() override
  {
    // Do safe delete with removing from list in case if DeleteNativeHttpThread
    // can produce final notifications to this->OnFinish().
    while (!m_threads.empty())
    {
      Thread * p = m_threads.back().first;
      m_threads.pop_back();
      thread::DeleteThread(p);
    }

    if (m_status == DownloadStatus::InProgress)
    {
      // means that client canceled download process, so delete all temporary files
      CloseWriter();

      if (m_doCleanProgressFiles)
      {
        Platform::RemoveFileIfExists(m_filePath + DOWNLOADING_FILE_EXTENSION);
        Platform::RemoveFileIfExists(m_filePath + RESUME_FILE_EXTENSION);
      }
    }
  }

  std::string const & GetData() const override { return m_filePath; }

private:
  ChunksDownloadStrategy::ResultT StartThreads()
  {
    std::string url;
    std::pair<int64_t, int64_t> range;
    ChunksDownloadStrategy::ResultT result;
    while ((result = m_strategy.NextChunk(url, range)) == ChunksDownloadStrategy::ENextChunk)
    {
      Thread * p = thread::CreateThread(url, *this, range.first, range.second, m_progress.m_bytesTotal);
      ASSERT(p, ());
      m_threads.emplace_back(p, range.first);
    }
    return result;
  }

  void RemoveHttpThreadByKey(int64_t begRange)
  {
    auto it = std::find_if(m_threads.begin(), m_threads.end(), ThreadByPos(begRange));
    if (it != m_threads.end())
    {
      Thread * p = it->first;
      m_threads.erase(it);
      thread::DeleteThread(p);
    }
    else
      LOG(LERROR, ("Tried to remove invalid thread for position", begRange));
  }

  bool OnWrite(int64_t offset, void const * buffer, size_t size) override
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

      m_strategy.SaveChunks(m_progress.m_bytesTotal, m_filePath + RESUME_FILE_EXTENSION);
    }
    catch (Writer::Exception const & e)
    {
      LOG(LWARNING, ("Can't flush writer", e.Msg()));
    }
  }

  /// Called for each chunk by one main (GUI) thread.
  void OnFinish(long httpOrErrorCode, int64_t begRange, int64_t endRange) override
  {
#ifdef DEBUG
    static threads::ThreadID const id = threads::GetCurrentThreadID();
    ASSERT_EQUAL(id, threads::GetCurrentThreadID(), ("OnFinish called from different threads"));
#endif

    bool const isChunkOk = (httpOrErrorCode == 200);
    UNUSED_VALUE(m_strategy.ChunkFinished(isChunkOk, {begRange, endRange}));

    // remove completed chunk from the list, beg is the key
    RemoveHttpThreadByKey(begRange);

    // report progress
    if (isChunkOk)
    {
      m_progress.m_bytesDownloaded += (endRange - begRange) + 1;
      if (m_onProgress)
        m_onProgress(*this);
    }
    else
    {
      auto const message = non_http_error_code::DebugPrint(httpOrErrorCode);
      LOG(LWARNING, (m_filePath, "HttpRequest error:", message));
    }

    ChunksDownloadStrategy::ResultT const result = StartThreads();
    if (result == ChunksDownloadStrategy::EDownloadFailed)
      m_status = httpOrErrorCode == 404 ? DownloadStatus::FileNotFound : DownloadStatus::Failed;
    else if (result == ChunksDownloadStrategy::EDownloadSucceeded)
      m_status = DownloadStatus::Completed;

    if (isChunkOk)
    {
      // save information for download resume
      ++m_goodChunksCount;
      if (m_status != DownloadStatus::Completed && m_goodChunksCount % 10 == 0)
        SaveResumeChunks();
    }

    if (m_status == DownloadStatus::InProgress)
      return;

    // 1. Save downloaded chunks if some error occured.
    if (m_status == DownloadStatus::Failed || m_status == DownloadStatus::FileNotFound)
      SaveResumeChunks();

    // 2. Free file handle.
    CloseWriter();

    // 3. Clean up resume file with chunks range on success
    if (m_status == DownloadStatus::Completed)
    {
      Platform::RemoveFileIfExists(m_filePath + RESUME_FILE_EXTENSION);

      // Rename finished file to it's original name.
      Platform::RemoveFileIfExists(m_filePath);
      base::RenameFileX(m_filePath + DOWNLOADING_FILE_EXTENSION, m_filePath);
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

      m_status = DownloadStatus::Failed;
    }
  }

  ChunksDownloadStrategy m_strategy;
  ThreadsContainerT m_threads;
  std::string m_filePath;
  std::unique_ptr<FileWriter> m_writer;

  size_t m_goodChunksCount;
  bool m_doCleanProgressFiles;
};
}  // namespace om::network::http::internal
