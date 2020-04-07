#include "storage/map_files_downloader.hpp"

#include "storage/queued_country.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"
#include "platform/servers_list.hpp"

#include "coding/url.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

namespace storage
{
void MapFilesDownloader::DownloadMapFile(QueuedCountry & queuedCountry)
{
  if (!m_serversList.empty())
  {
    Download(queuedCountry);
    return;
  }

  if (!m_isServersListRequested)
  {
    m_isServersListRequested = true;

    GetPlatform().RunTask(Platform::Thread::Network, [=]() mutable {
      GetServersList([=](ServersList const & serversList) mutable {
                      m_serversList = serversList;
                      m_quarantine.ForEachCountry([=](QueuedCountry & country) mutable {
                                                    Download(country);
                                                  });
                      m_quarantine.Clear();
                    });
    });
  }

  m_quarantine.Append(std::move(queuedCountry));
}

void MapFilesDownloader::Remove(CountryId const & id)
{
  if (m_quarantine.IsEmpty())
    return;

  m_quarantine.Remove(id);
}

void MapFilesDownloader::Clear()
{
  m_quarantine.Clear();
}

QueueInterface const & MapFilesDownloader::GetQueue() const
{
  return m_quarantine;
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
  return url::Join(baseUrl, OMIM_OS_NAME, strings::to_string(dataVersion),
                           url::UrlEncode(fileName));
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
    urls.emplace_back(url::Join(server, relativeUrl));

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
