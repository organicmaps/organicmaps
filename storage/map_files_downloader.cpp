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
  if (m_serverList.empty())
  {
    GetServersList([=](ServersList const & serverList)
                   {
                     m_serverList = serverList;
                     auto const urls = MakeUrlList(m_serverList, relativeUrl);
                     Download(urls, path, size, onDownloaded, onProgress);
                   });
  }
  else
  {
    auto const urls = MakeUrlList(m_serverList, relativeUrl);
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
std::string MapFilesDownloader::MakeFullUrlLegacy(string const & baseUrl, string const & fileName,
                                                  int64_t dataVersion)
{
  return base::url::Join(baseUrl, OMIM_OS_NAME, strings::to_string(dataVersion),
                         UrlEncode(fileName));
}

void MapFilesDownloader::SetServersList(ServersList const & serversList)
{
  m_serverList = serversList;
}

// static
MapFilesDownloader::ServersList MapFilesDownloader::LoadServersList()
{
  platform::HttpClient request(GetPlatform().MetaServerUrl());
  std::string httpResult;
  request.RunHttpRequest(httpResult);
  std::vector<std::string> urls;
  downloader::GetServerList(httpResult, urls);
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
