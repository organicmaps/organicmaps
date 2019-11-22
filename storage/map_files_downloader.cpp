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
void MapFilesDownloader::DownloadMapFile(QueuedCountry & queuedCountry,
                                         FileDownloadedCallback const & onDownloaded,
                                         DownloadingProgressCallback const & onProgress)
{
  if (!m_serversList.empty())
  {
    queuedCountry.ClarifyDownloadingType();
    Download(queuedCountry, onDownloaded, onProgress);
    return;
  }

  GetServersList([=](ServersList const & serversList) mutable
                 {
                   m_serversList = serversList;
                   queuedCountry.ClarifyDownloadingType();
                   Download(queuedCountry, onDownloaded, onProgress);
                 });
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

std::vector<std::string> MapFilesDownloader::MakeUrlList(std::string const & relativeUrl)
{
  std::vector<std::string> urls;
  urls.reserve(m_serversList.size());
  for (auto const & server : m_serversList)
    urls.emplace_back(base::url::Join(server, relativeUrl));

  return urls;
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
