#include "storage/storage_tests/fake_map_files_downloader.hpp"

#include "storage/storage_tests/task_runner.hpp"

#include "base/assert.hpp"
#include "base/scope_guard.hpp"

#include <algorithm>
#include <functional>
#include <utility>

namespace storage
{
int64_t const FakeMapFilesDownloader::kBlockSize;

FakeMapFilesDownloader::FakeMapFilesDownloader(TaskRunner & taskRunner) : m_timestamp(0), m_taskRunner(taskRunner)
{
  SetServersList({"http://test-url/"});
}

FakeMapFilesDownloader::~FakeMapFilesDownloader()
{
  CHECK_THREAD_CHECKER(m_checker, ());
}

void FakeMapFilesDownloader::Download(QueuedCountry && queuedCountry)
{
  CHECK_THREAD_CHECKER(m_checker, ());

  m_queue.Append(std::move(queuedCountry));

  if (m_queue.Count() == 1)
    Download();
}

void FakeMapFilesDownloader::Remove(CountryId const & id)
{
  CHECK_THREAD_CHECKER(m_checker, ());

  if (m_queue.IsEmpty())
    return;

  if (m_writer && m_queue.GetFirstId() == id)
    m_writer.reset();

  m_queue.Remove(id);

  ++m_timestamp;
}

void FakeMapFilesDownloader::Clear()
{
  CHECK_THREAD_CHECKER(m_checker, ());

  m_queue.Clear();
  m_writer.reset();
  ++m_timestamp;
}

QueueInterface const & FakeMapFilesDownloader::GetQueue() const
{
  CHECK_THREAD_CHECKER(m_checker, ());

  return m_queue;
}

void FakeMapFilesDownloader::Download()
{
  auto const & queuedCountry = m_queue.GetFirstCountry();
  if (!IsDownloadingAllowed())
  {
    OnFileDownloaded(queuedCountry, downloader::DownloadStatus::Failed);
    return;
  }

  queuedCountry.OnStartDownloading();

  ++m_timestamp;
  m_progress = {};
  m_progress.m_bytesTotal = queuedCountry.GetDownloadSize();
  m_writer.reset(new FileWriter(queuedCountry.GetFileDownloadPath()));
  m_taskRunner.PostTask(std::bind(&FakeMapFilesDownloader::DownloadNextChunk, this, m_timestamp));
}

void FakeMapFilesDownloader::DownloadNextChunk(uint64_t timestamp)
{
  CHECK_THREAD_CHECKER(m_checker, ());

  static std::string kZeroes(kBlockSize, '\0');

  if (timestamp != m_timestamp)
    return;

  ASSERT_LESS_OR_EQUAL(m_progress.m_bytesDownloaded, m_progress.m_bytesTotal, ());
  ASSERT(m_writer, ());

  if (m_progress.m_bytesDownloaded == m_progress.m_bytesTotal)
  {
    OnFileDownloaded(m_queue.GetFirstCountry(), downloader::DownloadStatus::Completed);
    return;
  }

  int64_t const bs = std::min(m_progress.m_bytesTotal - m_progress.m_bytesDownloaded, kBlockSize);

  m_progress.m_bytesDownloaded += bs;
  m_writer->Write(kZeroes.data(), bs);
  m_writer->Flush();

  m_taskRunner.PostTask([this, timestamp]()
  {
    CHECK_THREAD_CHECKER(m_checker, ());

    if (timestamp != m_timestamp)
      return;

    m_queue.GetFirstCountry().OnDownloadProgress(m_progress);
  });
  m_taskRunner.PostTask(std::bind(&FakeMapFilesDownloader::DownloadNextChunk, this, timestamp));
}

void FakeMapFilesDownloader::OnFileDownloaded(QueuedCountry const & queuedCountry,
                                              downloader::DownloadStatus const & status)
{
  auto const country = queuedCountry;
  m_queue.PopFront();

  m_taskRunner.PostTask([country, status]() { country.OnDownloadFinished(status); });

  if (!m_queue.IsEmpty())
    Download();
}
}  // namespace storage
