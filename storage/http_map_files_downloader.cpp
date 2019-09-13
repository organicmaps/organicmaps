#include "storage/http_map_files_downloader.hpp"

#include "platform/servers_list.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <functional>

using namespace std::placeholders;

namespace
{
class ErrorHttpRequest : public downloader::HttpRequest
{
public:
  explicit ErrorHttpRequest(std::string const & filePath)
  : HttpRequest(Callback(), Callback()), m_filePath(filePath)
  {
    m_status = Status::Failed;
  }

  virtual std::string const & GetData() const { return m_filePath; }

private:
  std::string m_filePath;
};
}  // anonymous namespace

namespace storage
{
HttpMapFilesDownloader::~HttpMapFilesDownloader()
{
  CHECK_THREAD_CHECKER(m_checker, ());
}

void HttpMapFilesDownloader::Download(std::vector<std::string> const & urls,
                                      std::string const & path, int64_t size,
                                      FileDownloadedCallback const & onDownloaded,
                                      DownloadingProgressCallback const & onProgress)
{
  CHECK_THREAD_CHECKER(m_checker, ());
  m_request.reset(downloader::HttpRequest::GetFile(
      urls, path, size,
      std::bind(&HttpMapFilesDownloader::OnMapFileDownloaded, this, onDownloaded, _1),
      std::bind(&HttpMapFilesDownloader::OnMapFileDownloadingProgress, this, onProgress, _1)));

  if (!m_request)
  {
    // Mark the end of download with error.
    ErrorHttpRequest error(path);
    OnMapFileDownloaded(onDownloaded, error);
  }
}

MapFilesDownloader::Progress HttpMapFilesDownloader::GetDownloadingProgress()
{
  CHECK_THREAD_CHECKER(m_checker, ());
  ASSERT(nullptr != m_request, ());
  return m_request->GetProgress();
}

bool HttpMapFilesDownloader::IsIdle()
{
  CHECK_THREAD_CHECKER(m_checker, ());
  return m_request.get() == nullptr;
}

void HttpMapFilesDownloader::Reset()
{
  CHECK_THREAD_CHECKER(m_checker, ());
  m_request.reset();
}

void HttpMapFilesDownloader::OnMapFileDownloaded(FileDownloadedCallback const & onDownloaded,
                                                 downloader::HttpRequest & request)
{
  CHECK_THREAD_CHECKER(m_checker, ());
  onDownloaded(request.GetStatus(), request.GetProgress());
}

void HttpMapFilesDownloader::OnMapFileDownloadingProgress(
    DownloadingProgressCallback const & onProgress, downloader::HttpRequest & request)
{
  CHECK_THREAD_CHECKER(m_checker, ());
  onProgress(request.GetProgress());
}
}  // namespace storage
