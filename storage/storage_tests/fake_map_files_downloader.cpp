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

FakeMapFilesDownloader::FakeMapFilesDownloader(TaskRunner & taskRunner)
  : m_progress(std::make_pair(0, 0)), m_timestamp(0), m_taskRunner(taskRunner)
{
  SetServersList({"http://test-url/"});
}

FakeMapFilesDownloader::~FakeMapFilesDownloader() { CHECK_THREAD_CHECKER(m_checker, ()); }

void FakeMapFilesDownloader::Download(QueuedCountry & queuedCountry)
{
  CHECK_THREAD_CHECKER(m_checker, ());

  m_queue.push_back(queuedCountry);

  if (m_queue.size() != 1)
    return;

  Download();
}

downloader::Progress FakeMapFilesDownloader::GetDownloadingProgress()
{
  CHECK_THREAD_CHECKER(m_checker, ());

  return m_progress;
}

bool FakeMapFilesDownloader::IsIdle()
{
  CHECK_THREAD_CHECKER(m_checker, ());

  return m_writer.get() == nullptr;
}

void FakeMapFilesDownloader::Pause()
{
  CHECK_THREAD_CHECKER(m_checker, ());

  m_writer.reset();
  ++m_timestamp;
}

void FakeMapFilesDownloader::Resume()
{
  CHECK_THREAD_CHECKER(m_checker, ());

  if (m_queue.empty() || m_writer)
    return;

  m_writer.reset(new FileWriter(m_queue.front().GetFileDownloadPath()));
  ++m_timestamp;
  m_taskRunner.PostTask(std::bind(&FakeMapFilesDownloader::DownloadNextChunk, this, m_timestamp));
}

void FakeMapFilesDownloader::Remove(CountryId const & id)
{
  CHECK_THREAD_CHECKER(m_checker, ());

  if (m_queue.empty())
    return;

  if (m_writer && m_queue.front() == id)
    m_writer.reset();

  auto it = std::find(m_queue.begin(), m_queue.end(), id);
  if (it != m_queue.end())
    m_queue.erase(it);

  ++m_timestamp;
}

void FakeMapFilesDownloader::Clear()
{
  CHECK_THREAD_CHECKER(m_checker, ());

  m_queue.clear();
  m_writer.reset();
  ++m_timestamp;
}

Queue const & FakeMapFilesDownloader::GetQueue() const
{
  CHECK_THREAD_CHECKER(m_checker, ());

  return m_queue;
}

void FakeMapFilesDownloader::Download()
{
  auto const & queuedCountry = m_queue.front();
  if (!IsDownloadingAllowed())
  {
    OnFileDownloaded(queuedCountry, downloader::DownloadStatus::Failed);
    return;
  }

  ++m_timestamp;
  m_progress = {0, queuedCountry.GetDownloadSize()};
  m_writer.reset(new FileWriter(queuedCountry.GetFileDownloadPath()));
  m_taskRunner.PostTask(std::bind(&FakeMapFilesDownloader::DownloadNextChunk, this, m_timestamp));
}

void FakeMapFilesDownloader::DownloadNextChunk(uint64_t timestamp)
{
  CHECK_THREAD_CHECKER(m_checker, ());

  static std::string kZeroes(kBlockSize, '\0');

  if (timestamp != m_timestamp)
    return;

  ASSERT_LESS_OR_EQUAL(m_progress.first, m_progress.second, ());
  ASSERT(m_writer, ());

  if (m_progress.first == m_progress.second)
  {
    OnFileDownloaded(m_queue.front(), downloader::DownloadStatus::Completed);
    return;
  }

  int64_t const bs = std::min(m_progress.second - m_progress.first, kBlockSize);

  m_progress.first += bs;
  m_writer->Write(kZeroes.data(), bs);
  m_writer->Flush();

  m_taskRunner.PostTask([this, timestamp]()
  {
    CHECK_THREAD_CHECKER(m_checker, ());

    if (timestamp != m_timestamp)
      return;

    m_queue.front().OnDownloadProgress(m_progress);
  });
  m_taskRunner.PostTask(std::bind(&FakeMapFilesDownloader::DownloadNextChunk, this, timestamp));
}

void FakeMapFilesDownloader::OnFileDownloaded(QueuedCountry const & queuedCountry,
                                              downloader::DownloadStatus const & status)
{
  auto const country = queuedCountry;
  m_queue.pop_front();

  m_taskRunner.PostTask([country, status]() { country.OnDownloadFinished(status); });

  if (!m_queue.empty())
    Download();
}
}  // namespace storage
