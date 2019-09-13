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
    : m_progress(std::make_pair(0, 0)), m_idle(true), m_timestamp(0), m_taskRunner(taskRunner)
{
  SetServersList({"http://test-url/"});
}

FakeMapFilesDownloader::~FakeMapFilesDownloader() { CHECK(m_checker.CalledOnOriginalThread(), ()); }

void FakeMapFilesDownloader::Download(std::vector<std::string> const & urls,
                                      std::string const & path, int64_t size,
                                      FileDownloadedCallback const & onDownloaded,
                                      DownloadingProgressCallback const & onProgress)
{
  CHECK(m_checker.CalledOnOriginalThread(), ());

  m_progress.first = 0;
  m_progress.second = size;
  m_idle = false;

  m_writer.reset(new FileWriter(path));
  m_onDownloaded = onDownloaded;
  m_onProgress = onProgress;

  ++m_timestamp;
  m_taskRunner.PostTask(std::bind(&FakeMapFilesDownloader::DownloadNextChunk, this, m_timestamp));
}

MapFilesDownloader::Progress FakeMapFilesDownloader::GetDownloadingProgress()
{
  CHECK(m_checker.CalledOnOriginalThread(), ());
  return m_progress;
}

bool FakeMapFilesDownloader::IsIdle()
{
  CHECK(m_checker.CalledOnOriginalThread(), ());
  return m_idle;
}

void FakeMapFilesDownloader::Reset()
{
  CHECK(m_checker.CalledOnOriginalThread(), ());
  m_idle = true;
  m_writer.reset();
  ++m_timestamp;
}

void FakeMapFilesDownloader::DownloadNextChunk(uint64_t timestamp)
{
  CHECK(m_checker.CalledOnOriginalThread(), ());

  static std::string kZeroes(kBlockSize, '\0');

  if (timestamp != m_timestamp)
    return;

  ASSERT_LESS_OR_EQUAL(m_progress.first, m_progress.second, ());
  ASSERT(m_writer, ());

  if (m_progress.first == m_progress.second)
  {
    m_taskRunner.PostTask(std::bind(m_onDownloaded, downloader::HttpRequest::Status::Completed, m_progress));
    Reset();
    return;
  }

  int64_t const bs = std::min(m_progress.second - m_progress.first, kBlockSize);

  m_progress.first += bs;
  m_writer->Write(kZeroes.data(), bs);
  m_writer->Flush();

  m_taskRunner.PostTask(std::bind(m_onProgress, m_progress));
  m_taskRunner.PostTask(std::bind(&FakeMapFilesDownloader::DownloadNextChunk, this, timestamp));
}
}  // namespace storage
