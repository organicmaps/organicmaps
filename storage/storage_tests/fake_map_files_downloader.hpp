#pragma once

#include "storage/map_files_downloader.hpp"

#include "coding/file_writer.hpp"

#include "base/thread_checker.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace storage
{
class TaskRunner;

// This class can be used in tests to mimic a real downloader.  It
// always returns a single URL for map files downloading and when
// asked for a file, creates a file with zero-bytes content on a disk.
// Because all callbacks must be invoked asynchronously, it needs a
// single-thread message loop runner to run callbacks.
//
// *NOTE*, this class is not thread-safe.
class FakeMapFilesDownloader : public MapFilesDownloader
{
public:
  static int64_t const kBlockSize = 1024 * 1024;

  FakeMapFilesDownloader(TaskRunner & taskRunner);

  ~FakeMapFilesDownloader();

  Progress GetDownloadingProgress() override;
  bool IsIdle() override;
  void Reset() override;

private:
  // MapFilesDownloader overrides:
  void Download(std::vector<std::string> const & urls, std::string const & path, int64_t size,
                FileDownloadedCallback const & onDownloaded,
                DownloadingProgressCallback const & onProgress) override;

  void DownloadNextChunk(uint64_t requestId);

  Progress m_progress;
  bool m_idle;

  std::unique_ptr<FileWriter> m_writer;
  FileDownloadedCallback m_onDownloaded;
  DownloadingProgressCallback m_onProgress;

  uint64_t m_timestamp;

  TaskRunner & m_taskRunner;
  ThreadChecker m_checker;
};
}  // namespace storage
