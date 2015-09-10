#include "storage/http_map_files_downloader.hpp"

#include "platform/platform.hpp"
#include "platform/servers_list.hpp"

#include "base/assert.hpp"

#include "std/bind.hpp"
#include "base/string_utils.hpp"

namespace storage
{
HttpMapFilesDownloader::~HttpMapFilesDownloader()
{
  ASSERT(m_checker.CalledOnOriginalThread(), ());
}

void HttpMapFilesDownloader::GetServersList(int64_t const mapVersion, string const & mapFileName,
                                            TServersListCallback const & callback)
{
  ASSERT(m_checker.CalledOnOriginalThread(), ());
  m_request.reset(downloader::HttpRequest::PostJson(
      GetPlatform().MetaServerUrl(), strings::to_string(mapVersion) + '/' + mapFileName,
      bind(&HttpMapFilesDownloader::OnServersListDownloaded, this, callback, _1)));
}

void HttpMapFilesDownloader::DownloadMapFile(vector<string> const & urls, string const & path,
                                             int64_t size,
                                             TFileDownloadedCallback const & onDownloaded,
                                             TDownloadingProgressCallback const & onProgress)
{
  ASSERT(m_checker.CalledOnOriginalThread(), ());
  m_request.reset(downloader::HttpRequest::GetFile(
      urls, path, size, bind(&HttpMapFilesDownloader::OnMapFileDownloaded, this, onDownloaded, _1),
      bind(&HttpMapFilesDownloader::OnMapFileDownloadingProgress, this, onProgress, _1)));
}

MapFilesDownloader::TProgress HttpMapFilesDownloader::GetDownloadingProgress()
{
  ASSERT(m_checker.CalledOnOriginalThread(), ());
  return m_request->Progress();
}

bool HttpMapFilesDownloader::IsIdle()
{
  ASSERT(m_checker.CalledOnOriginalThread(), ());
  return m_request.get() == nullptr;
}

void HttpMapFilesDownloader::Reset()
{
  ASSERT(m_checker.CalledOnOriginalThread(), ());
  m_request.reset();
}

void HttpMapFilesDownloader::OnServersListDownloaded(TServersListCallback const & callback,
                                                     downloader::HttpRequest & request)
{
  ASSERT(m_checker.CalledOnOriginalThread(), ());
  vector<string> urls;
  GetServerListFromRequest(request, urls);
  callback(urls);
}

void HttpMapFilesDownloader::OnMapFileDownloaded(TFileDownloadedCallback const & onDownloaded,
                                                 downloader::HttpRequest & request)
{
  ASSERT(m_checker.CalledOnOriginalThread(), ());
  bool const success = request.Status() != downloader::HttpRequest::EFailed;
  onDownloaded(success, request.Progress());
}

void HttpMapFilesDownloader::OnMapFileDownloadingProgress(
    TDownloadingProgressCallback const & onProgress, downloader::HttpRequest & request)
{
  ASSERT(m_checker.CalledOnOriginalThread(), ());
  onProgress(request.Progress());
}
}  // namespace storage
