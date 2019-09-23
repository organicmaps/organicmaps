#include "storage/map_files_downloader.hpp"

#include "storage/queued_country.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"
#include "platform/servers_list.hpp"

#include "coding/url_encode.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"
#include "base/url_helpers.hpp"

#include <sstream>

namespace storage
{
namespace
{
std::vector<std::string> MakeUrlList(MapFilesDownloader::ServersList const & servers,
                                     std::string const & relativeUrl)
{
  std::vector<std::string> urls;
  urls.reserve(servers.size());
  for (auto const & server : servers)
    urls.emplace_back(base::url::Join(server, relativeUrl));

  return urls;
}
}  // namespace

void MapFilesDownloader::DownloadMapFile(std::string const & relativeUrl,
                                         std::string const & path, int64_t size,
                                         FileDownloadedCallback const & onDownloaded,
                                         DownloadingProgressCallback const & onProgress)
{
  if (m_serversList.empty())
  {
    GetServersList([=](ServersList const & serversList)
                   {
                     m_serversList = serversList;
                     auto const urls = MakeUrlList(m_serversList, relativeUrl);
                     Download(urls, path, size, onDownloaded, onProgress);
                   });
  }
  else
  {
    auto const urls = MakeUrlList(m_serversList, relativeUrl);
    Download(urls, path, size, onDownloaded, onProgress);
  }
}

// static
std::string MapFilesDownloader::MakeRelativeUrl(std::string const & fileName, int64_t dataVersion,
                                                uint64_t diffVersion)
{
  std::ostringstream url;
  if (diffVersion != 0)
    url << "diffs/" << dataVersion << "/" << diffVersion;
  else
    url << OMIM_OS_NAME "/" << dataVersion;

  return base::url::Join(url.str(), UrlEncode(fileName));
}

// static
std::string MapFilesDownloader::MakeFullUrlLegacy(std::string const & baseUrl,
                                                  std::string const & fileName, int64_t dataVersion)
{
  return base::url::Join(baseUrl, OMIM_OS_NAME, strings::to_string(dataVersion),
                         UrlEncode(fileName));
}

void MapFilesDownloader::SetServersList(ServersList const & serversList)
{
  m_serversList = serversList;
}

void MapFilesDownloader::SetDiffs(diffs::NameDiffInfoMap const & diffs)
{
  m_diffs = diffs;
}

// static
MapFilesDownloader::ServersList MapFilesDownloader::LoadServersList()
{
  auto constexpr kTimeoutInSeconds = 10.0;

  platform::HttpClient request(GetPlatform().MetaServerUrl());
  std::string httpResult;
  request.SetTimeout(kTimeoutInSeconds);
  request.RunHttpRequest(httpResult);
  std::vector<std::string> urls;
  downloader::GetServersList(httpResult, urls);
  CHECK(!urls.empty(), ());
  return urls;
}

void MapFilesDownloader::GetServersList(ServersListCallback const & callback)
{
  GetPlatform().RunTask(Platform::Thread::Network, [callback]()
  {
    callback(LoadServersList());
  });
}
}  // namespace storage
