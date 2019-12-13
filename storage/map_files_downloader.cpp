#include "storage/map_files_downloader.hpp"

#include "storage/queued_country.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"
#include "platform/servers_list.hpp"

#include "coding/url_encode.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"
#include "base/url_helpers.hpp"

namespace storage
{
void MapFilesDownloader::DownloadMapFile(QueuedCountry & queuedCountry)
{
  // The goal is to call Download(queuedCountry) on the current (UI) thread
  // after the server list has been received on the network thread.

  // Fast path: the server list was received before.
  if (!m_serversList.empty())
  {
    queuedCountry.ClarifyDownloadingType();
    Download(queuedCountry);
    return;
  }

  // Slow path: until we know the servers list, we have to route all
  // requests through the network thread, thus implicitly synchronizing them.
  if (!m_isServersListRequested)
  {
    m_isServersListRequested = true;

    GetPlatform().RunTask(Platform::Thread::Network, [=]() mutable {
      GetServersList([=](ServersList const & serversList) mutable {
                       m_serversList = serversList;
                     });
    });
  }

  GetPlatform().RunTask(Platform::Thread::Network, [=]() mutable {
    // This task has arrived to the network thread after GetServersList
    // synchronously downloaded the server list.
    // It is now safe to repost the download task to the UI thread.
      GetPlatform().RunTask(Platform::Thread::Gui, [=]() mutable {
        queuedCountry.ClarifyDownloadingType();
        Download(queuedCountry);
      });
  });
}

void MapFilesDownloader::Subscribe(Subscriber * subscriber)
{
  m_subscribers.push_back(subscriber);
}

void MapFilesDownloader::UnsubscribeAll()
{
  m_subscribers.clear();
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

void MapFilesDownloader::SetDownloadingPolicy(DownloadingPolicy * policy)
{
  m_downloadingPolicy = policy;
}

bool MapFilesDownloader::IsDownloadingAllowed() const
{
  return m_downloadingPolicy == nullptr || m_downloadingPolicy->IsDownloadingAllowed();
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
  callback(LoadServersList());
}
}  // namespace storage
