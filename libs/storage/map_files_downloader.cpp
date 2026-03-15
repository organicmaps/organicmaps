#include "storage/map_files_downloader.hpp"

#include "storage/queued_country.hpp"

#include "platform/downloader_utils.hpp"
#include "platform/http_client.hpp"
#include "platform/locale.hpp"
#include "platform/platform.hpp"
#include "platform/products.hpp"
#include "platform/servers_list.hpp"
#include "platform/settings.hpp"

#include "coding/url.hpp"

#include "base/assert.hpp"

namespace storage
{
MapFilesDownloader::~MapFilesDownloader()
{
  m_alive->store(false, std::memory_order_release);
  ++m_generation;
  m_metaConfigWaiters.clear();
  if (m_downloadHandle)
    m_downloadHandle->Cancel();
}

void MapFilesDownloader::DownloadMapFile(QueuedCountry && queuedCountry)
{
  if (!m_serversList.empty())
  {
    Download(std::move(queuedCountry));
    return;
  }

  m_pendingRequests.Append(std::move(queuedCountry));

  EnsureMetaConfigReady([this, alive = m_alive]()
  {
    if (!alive->load(std::memory_order_acquire))
      return;
    m_pendingRequests.ForEachCountry([this](QueuedCountry & country) { Download(std::move(country)); });
    m_pendingRequests.Clear();
  });
}

void MapFilesDownloader::RunMetaConfigAsync()
{
  m_isMetaConfigRequested = true;

  GetPlatform().RunTask(Platform::Thread::Network, [this, alive = m_alive]()
  {
    if (!alive->load(std::memory_order_acquire))
      return;
    auto metaConfig = GetMetaConfig();

    // Thread-safe.
    settings::Update(metaConfig.settings);
    products::ProductsSettings::Instance().Update(std::move(metaConfig.productsConfig));

    GetPlatform().RunTask(Platform::Thread::Gui, [this, alive, servers = metaConfig.servers]()
    {
      if (!alive->load(std::memory_order_acquire))
        return;
      m_serversList = std::move(servers);

      // Drain all queued waiters (from DownloadMapFile and DownloadAsString).
      auto waiters = std::move(m_metaConfigWaiters);
      for (auto & w : waiters)
        w();

      // Reset flag to invoke servers list downloading next time if current request has failed.
      m_isMetaConfigRequested = false;
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

void MapFilesDownloader::DownloadAsString(std::string url, std::function<bool(std::string const &)> && callback,
                                          bool forceReset /* = false */)
{
  EnsureMetaConfigReady([this, alive = m_alive, forceReset, url = std::move(url), callback = std::move(callback)]()
  {
    if (!alive->load(std::memory_order_acquire))
      return;

    if ((m_downloadHandle && !forceReset) || m_serversList.empty())
      return;

    if (m_downloadHandle)
      m_downloadHandle->Cancel();

    ++m_generation;

    // Servers are sorted from best to worst.
    platform::HttpClient client(url::Join(m_serversList.front(), url));
    m_downloadHandle = client.RunHttpRequestAsync(
        [this, alive, gen = m_generation, callback = std::move(callback)](platform::HttpClient::Result result)
    {
      GetPlatform().RunTask(Platform::Thread::Gui,
                            [this, alive, gen, callback = std::move(callback), result = std::move(result)]()
      {
        if (!alive->load(std::memory_order_acquire) || gen != m_generation)
          return;

        bool keepHandle = false;
        if (result.m_success && result.m_errorCode == 200 && !result.m_serverResponse.empty())
          keepHandle = callback(result.m_serverResponse);

        if (!keepHandle)
          m_downloadHandle.reset();
      });
    });
  });
}

void MapFilesDownloader::EnsureMetaConfigReady(std::function<void()> && callback)
{
  /// @todo Implement logic if m_metaConfig is "outdated".
  /// Fetch new servers list on each download request?
  if (!m_serversList.empty())
  {
    callback();
    return;
  }

  m_metaConfigWaiters.push_back(std::move(callback));

  if (!m_isMetaConfigRequested)
    RunMetaConfigAsync();
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

std::string GetAcceptLanguage()
{
  auto const locale = platform::GetCurrentLocale();
  return locale.m_language + "-" + locale.m_country;
}

downloader::MetaConfig MapFilesDownloader::LoadMetaConfig() const
{
  Platform & pl = GetPlatform();
  std::string const metaServerUrl = pl.MetaServerUrl();

  std::optional<downloader::MetaConfig> metaConfig;
  if (!metaServerUrl.empty())
  {
    platform::HttpClient request(metaServerUrl);
    request.SetRawHeader("X-OM-DataVersion", std::to_string(m_dataVersion));

    request.SetRawHeader("X-OM-AppVersion", pl.Version());
    request.SetRawHeader("Accept-Language", GetAcceptLanguage());

    /// @DebugNote Uncomment to check donates flow.
    // request.SetRawHeader("X-OM-AppVersion", "2025.09.19-6-ios");  // "2025.09.15-18-FDroid"
    // request.SetRawHeader("Accept-Language", "de-DE");

    request.SetTimeout(10.0);  // timeout in seconds

    std::string httpResult;
    request.RunHttpRequest(httpResult);
    metaConfig = downloader::ParseMetaConfig(httpResult);
  }

  if (!metaConfig)
  {
    metaConfig = downloader::ParseMetaConfig(pl.DefaultUrlsJSON());
    CHECK(metaConfig, ());
    LOG(LWARNING, ("Can't get meta configuration from request, using default servers:", metaConfig->servers));
  }

  return *metaConfig;
}

downloader::MetaConfig MapFilesDownloader::GetMetaConfig()
{
  return LoadMetaConfig();
}

}  // namespace storage
