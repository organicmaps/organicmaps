#include "storage/http_map_files_downloader.hpp"

#include "platform/platform.hpp"
#include "platform/servers_list.hpp"

#include "base/assert.hpp"

#include "std/bind.hpp"
#include "base/string_utils.hpp"

namespace
{
class ErrorHttpRequest : public downloader::HttpRequest
{
  string m_filePath;
public:
  ErrorHttpRequest(string const & filePath)
  : HttpRequest(CallbackT(), CallbackT()), m_filePath(filePath)
  {
    m_status = EFailed;
  }

  virtual string const & Data() const { return m_filePath; }
};
}  // anonymous namespace

namespace storage
{
HttpMapFilesDownloader::~HttpMapFilesDownloader()
{
  ASSERT_THREAD_CHECKER(m_checker, ());
}

void HttpMapFilesDownloader::GetServersList(TServersListCallback const & callback)
{
  ASSERT_THREAD_CHECKER(m_checker, ());
  m_request.reset(downloader::HttpRequest::Get(
      GetPlatform().MetaServerUrl(),
      bind(&HttpMapFilesDownloader::OnServersListDownloaded, this, callback, _1)));
}

void HttpMapFilesDownloader::DownloadMapFile(vector<string> const & urls, string const & path,
                                             int64_t size,
                                             TFileDownloadedCallback const & onDownloaded,
                                             TDownloadingProgressCallback const & onProgress)
{
  ASSERT_THREAD_CHECKER(m_checker, ());
  m_request.reset(downloader::HttpRequest::GetFile(
      urls, path, size, bind(&HttpMapFilesDownloader::OnMapFileDownloaded, this, onDownloaded, _1),
      bind(&HttpMapFilesDownloader::OnMapFileDownloadingProgress, this, onProgress, _1)));

  if (!m_request)
  {
    // Mark the end of download with error.
    ErrorHttpRequest error(path);
    OnMapFileDownloaded(onDownloaded, error);
  }
}

MapFilesDownloader::TProgress HttpMapFilesDownloader::GetDownloadingProgress()
{
  ASSERT_THREAD_CHECKER(m_checker, ());
  ASSERT(nullptr != m_request, ());
  return m_request->Progress();
}

bool HttpMapFilesDownloader::IsIdle()
{
  ASSERT_THREAD_CHECKER(m_checker, ());
  return m_request.get() == nullptr;
}

void HttpMapFilesDownloader::Reset()
{
  ASSERT_THREAD_CHECKER(m_checker, ());
  m_request.reset();
}

void HttpMapFilesDownloader::OnServersListDownloaded(TServersListCallback const & callback,
                                                     downloader::HttpRequest & request)
{
  ASSERT_THREAD_CHECKER(m_checker, ());
  vector<string> urls;
  GetServerListFromRequest(request, urls);
  callback(urls);
}

void HttpMapFilesDownloader::OnMapFileDownloaded(TFileDownloadedCallback const & onDownloaded,
                                                 downloader::HttpRequest & request)
{
  ASSERT_THREAD_CHECKER(m_checker, ());
  bool const success = request.Status() != downloader::HttpRequest::EFailed;
  onDownloaded(success, request.Progress());
}

void HttpMapFilesDownloader::OnMapFileDownloadingProgress(
    TDownloadingProgressCallback const & onProgress, downloader::HttpRequest & request)
{
  ASSERT_THREAD_CHECKER(m_checker, ());
  onProgress(request.Progress());
}
}  // namespace storage
