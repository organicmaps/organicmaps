#include "storage/storage_tests/fake_map_files_downloader.hpp"

#include "storage/storage_tests/task_runner.hpp"

#include "coding/file_writer.hpp"

#include "base/assert.hpp"
#include "base/scope_guard.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"

namespace storage
{
namespace
{
int64_t const kBlockSize = 1024 * 1024;
}  // namespace

FakeMapFilesDownloader::FakeMapFilesDownloader(TaskRunner & taskRunner)
    : m_progress(make_pair(0, 0)), m_idle(true), m_taskRunner(taskRunner)
{
  m_servers.push_back("http://test-url/");
}

FakeMapFilesDownloader::~FakeMapFilesDownloader() { CHECK(m_checker.CalledOnOriginalThread(), ()); }

void FakeMapFilesDownloader::GetServersList(string const & mapFileName,
                                            TServersListCallback const & callback)
{
  CHECK(m_checker.CalledOnOriginalThread(), ());
  m_idle = false;
  MY_SCOPE_GUARD(resetIdle, bind(&FakeMapFilesDownloader::Reset, this));
  m_taskRunner.PostTask(bind(callback, m_servers));
}

void FakeMapFilesDownloader::DownloadMapFile(vector<string> const & urls, string const & path,
                                             int64_t size,
                                             TFileDownloadedCallback const & onDownloaded,
                                             TDownloadingProgressCallback const & onProgress)
{
  static string kZeroes(kBlockSize, '\0');

  m_progress.first = 0;
  m_progress.second = size;
  m_idle = false;
  MY_SCOPE_GUARD(resetIdle, bind(&FakeMapFilesDownloader::Reset, this));

  {
    FileWriter writer(path);
    while (size != 0)
    {
      int64_t const blockSize = min(size, kBlockSize);
      writer.Write(kZeroes.data(), blockSize);
      size -= blockSize;
      m_progress.first += blockSize;
      m_taskRunner.PostTask(bind(onProgress, m_progress));
    }
  }
  m_taskRunner.PostTask(bind(onDownloaded, true /* success */, m_progress));
}

MapFilesDownloader::TProgress FakeMapFilesDownloader::GetDownloadingProgress()
{
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
}

}  // namespace storage
