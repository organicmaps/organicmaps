#pragma once

#include "storage/downloader_queue_universal.hpp"
#include "storage/map_files_downloader.hpp"
#include "storage/queued_country.hpp"

#include "platform/downloader_defines.hpp"

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

  // MapFilesDownloader overrides:
  void Remove(CountryId const & id) override;
  void Clear() override;
  QueueInterface const & GetQueue() const override;

private:
  // MapFilesDownloader overrides:
  void Download(QueuedCountry && queuedCountry) override;

  void Download();
  void DownloadNextChunk(uint64_t requestId);
  void OnFileDownloaded(QueuedCountry const & queuedCountry, downloader::DownloadStatus const & status);

  downloader::Progress m_progress;

  std::unique_ptr<FileWriter> m_writer;

  uint64_t m_timestamp;

  TaskRunner & m_taskRunner;
  ThreadChecker m_checker;
  Queue m_queue;
};
}  // namespace storage
