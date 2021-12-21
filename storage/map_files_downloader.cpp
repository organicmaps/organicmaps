#include "storage/map_files_downloader.hpp"

#include "storage/queued_country.hpp"

#include "platform/downloader_utils.hpp"
#include "platform/http_client.hpp"
#include "platform/platform.hpp"
#include "platform/servers_list.hpp"

#include "coding/url.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

namespace storage
{
void MapFilesDownloader::DownloadMapFile(QueuedCountry && queuedCountry)
{
  if (!m_serversList.empty())
  {
    Download(std::move(queuedCountry));
    return;
  }

  m_pendingRequests.Append(std::move(queuedCountry));

  if (!m_isServersListRequested)
  {
    RunServersListAsync([this]()
    {
      m_pendingRequests.ForEachCountry([this](QueuedCountry & country)
      {
        Download(std::move(country));
      });

      m_pendingRequests.Clear();
    });
  }
}

void MapFilesDownloader::RunServersListAsync(std::function<void()> && callback)
{
  m_isServersListRequested = true;

  GetPlatform().RunTask(Platform::Thread::Network, [this, callback = std::move(callback)]()
  {
    GetServersList([this, callback = std::move(callback)](ServersList const & serversList)
    {
      m_serversList = serversList;

      callback();

      // Reset flag to invoke servers list downloading next time if current request has failed.
      m_isServersListRequested = false;
    });
  });
}

void MapFilesDownloader::Remove(CountryId const & id)
{
  if (!m_pendingRequests.IsEmpty())
    m_pendingRequests.Remove(id);
}

void MapFilesDownloader::Clear()
{
  m_pendingRequests.Clear();
}

QueueInterface const & MapFilesDownloader::GetQueue() const
{
  return m_pendingRequests;
}

void MapFilesDownloader::DownloadAsString(std::string url, std::function<bool (std::string const &)> && callback,
                                          bool forceReset /* = false */)
{
  EnsureServersListReady([this, forceReset, url = std::move(url), callback = std::move(callback)]()
  {
    if ((m_fileRequest && !forceReset) || m_serversList.empty())
      return;

    // Servers are sorted from best to worst.
    m_fileRequest.reset(RequestT::Get(url::Join(m_serversList.front(), url),
      [this, callback = std::move(callback)](RequestT & request)
      {
        bool deleteRequest = true;

        auto const & buffer = request.GetData();
        if (!buffer.empty())
        {
          // Update deleteRequest flag if new download was requested in callback.
          deleteRequest = !callback(buffer);
        }

        if (deleteRequest)
          m_fileRequest.reset();
      }));
  });
}

void MapFilesDownloader::EnsureServersListReady(std::function<void ()> && callback)
{
  /// @todo Implement logic if m_serversList is "outdated".
  /// Fetch new servers list on each download request?
  if (!m_serversList.empty())
  {
    callback();
  }
  else if (!m_isServersListRequested)
  {
    RunServersListAsync(std::move(callback));
  }
  else
  {
    // skip this request without callback call
  }
}

std::vector<std::string> MapFilesDownloader::MakeUrlListLegacy(std::string const & fileName) const
{
  return MakeUrlList(downloader::GetFileDownloadUrl(fileName, m_dataVersion));
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

std::vector<std::string> MapFilesDownloader::MakeUrlList(std::string const & relativeUrl) const
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
  std::string const metaServerUrl = GetPlatform().MetaServerUrl();
  std::string httpResult;

  if (!metaServerUrl.empty())
  {
    platform::HttpClient request(metaServerUrl);
    request.SetRawHeader("X-OM-DataVersion", std::to_string(m_dataVersion));
    request.SetTimeout(10.0); // timeout in seconds
    request.RunHttpRequest(httpResult);
  }

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
