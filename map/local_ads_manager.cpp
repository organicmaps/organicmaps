#include "map/local_ads_manager.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/scales.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"

#include "base/bits.hpp"

namespace
{
std::string const kCampaignFile = "local_ads_campaigns.dat";
std::string const kExpirationFile = "local_ads_expiration.dat";
auto constexpr kWWanUpdateTimeout = std::chrono::hours(12);

std::vector<uint8_t> SerializeTimestamp(std::chrono::steady_clock::time_point ts)
{
  auto hours = std::chrono::duration_cast<std::chrono::hours>(ts.time_since_epoch()).count();
  uint64_t const encodedHours = bits::ZigZagEncode(static_cast<int64_t>(hours));

  std::vector<uint8_t> result;
  MemWriter<decltype(result)> writer(result);
  writer.Write(&encodedHours, sizeof(encodedHours));

  return result;
}

std::chrono::steady_clock::time_point DeserializeTimestamp(ModelReaderPtr const & reader)
{
  ASSERT_EQUAL(reader.Size(), sizeof(long), ());

  std::vector<uint8_t> bytes(reader.Size());
  reader.Read(0, bytes.data(), bytes.size());

  MemReaderWithExceptions memReader(bytes.data(), bytes.size());
  ReaderSource<decltype(memReader)> src(memReader);
  uint64_t hours = ReadPrimitiveFromSource<uint64_t>(src);
  int64_t const decodedHours = bits::ZigZagDecode(hours);

  return std::chrono::steady_clock::time_point(std::chrono::hours(decodedHours));
}

std::string GetPath(std::string const & fileName)
{
  return my::JoinFoldersToPath(GetPlatform().WritableDir(), fileName);
}
}  // namespace

LocalAdsManager::LocalAdsManager(GetMwmsByRectFn const & getMwmsByRectFn,
                       GetMwmIdByName const & getMwmIdByName)
  : m_getMwmsByRectFn(getMwmsByRectFn)
  , m_getMwmIdByNameFn(getMwmIdByName)
{
  CHECK(m_getMwmsByRectFn != nullptr, ());
  CHECK(m_getMwmIdByNameFn != nullptr, ());
}

LocalAdsManager::~LocalAdsManager()
{
#ifdef DEBUG
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    ASSERT(!m_isRunning, ());
  }
#endif
}

void LocalAdsManager::Startup()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isRunning)
      return;
    m_isRunning = true;
  }
  m_thread = threads::SimpleThread(&LocalAdsManager::ThreadRoutine, this);
}

void LocalAdsManager::Teardown()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_isRunning)
      return;
    m_isRunning = false;
  }
  m_condition.notify_one();
  m_thread.join();
}

void LocalAdsManager::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  m_drapeEngine = engine;
  {
    std::lock_guard<std::mutex> lock(m_symbolsCacheMutex);
    if (!m_symbolsCache.empty())
      m_drapeEngine->AddCustomSymbols(std::move(m_symbolsCache));
  }
}

void LocalAdsManager::UpdateViewport(ScreenBase const & screen)
{
  auto connectionStatus = GetPlatform().ConnectionStatus();
  if (connectionStatus == Platform::EConnectionType::CONNECTION_NONE ||
      df::GetZoomLevel(screen.GetScale()) <= scales::GetUpperWorldScale())
  {
    return;
  }

  std::vector<std::string> requestedCampaigns;
  auto mwms = m_getMwmsByRectFn(screen.ClipRect());
  if (mwms.empty())
    return;

  // Request local ads campaigns.
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto const & mwm : mwms)
    {
      auto info = mwm.GetInfo();
      if (!info)
        continue;
      std::string const & mwmName = info->GetCountryName();
      auto campaignIt = m_campaigns.find(mwmName);
      if (campaignIt == m_campaigns.end())
      {
        requestedCampaigns.push_back(mwmName);
        continue;
      }

      // If a campaign has not been requested from server this session.
      if (!campaignIt->second)
      {
        auto const it = m_expiration.find(mwmName);
        bool needUpdateByTimeout = (connectionStatus == Platform::EConnectionType::CONNECTION_WIFI);
        if (!needUpdateByTimeout && it != m_expiration.end())
        {
          auto const currentTime = std::chrono::steady_clock::now();
          needUpdateByTimeout = currentTime > (it->second + kWWanUpdateTimeout);
        }

        if (needUpdateByTimeout || it == m_expiration.end())
          requestedCampaigns.push_back(mwmName);
      }
    }

    if (!requestedCampaigns.empty())
    {
      m_requestedCampaigns.reserve(m_requestedCampaigns.size() + requestedCampaigns.size());
      for (auto const & campaign : requestedCampaigns)
        m_requestedCampaigns.push_back(std::make_pair(m_getMwmIdByNameFn(campaign), RequestType::Download));
      m_condition.notify_one();
    }
  }
}

void LocalAdsManager::ThreadRoutine()
{
  std::string const campaignFile = GetPath(kCampaignFile);
  std::string const expirationFile = GetPath(kExpirationFile);

  // Read persistence data (expiration file must be read first).
  ReadExpirationFile(expirationFile);
  ReadCampaignFile(campaignFile);

  std::vector<Request> campaignMwms;
  while (WaitForRequest(campaignMwms))
  {
    try
    {
      FilesContainerW campaignsContainer(campaignFile, FileWriter::OP_WRITE_EXISTING);
      FilesContainerW expirationContainer(expirationFile, FileWriter::OP_WRITE_EXISTING);
      for (auto const & mwm : campaignMwms)
      {
        if (!mwm.first.IsAlive())
          continue;

        std::string const countryName = mwm.first.GetInfo()->GetCountryName();
        if (mwm.second == RequestType::Download)
        {
          // Download campaign data from server.
          std::vector<uint8_t> rawData = DownloadCampaign(mwm.first);
          if (rawData.empty())
            continue;

          // Save data persistently.
          campaignsContainer.Write(rawData, countryName);
          auto ts = std::chrono::steady_clock::now();
          expirationContainer.Write(SerializeTimestamp(ts), countryName);

          // Update run-time data.
          {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_campaigns[countryName] = true;
            m_expiration[countryName] = ts;
          }

          // Deserialize and send data to rendering.
          auto symbols = DeserializeCampaign(std::move(rawData), ts);
          if (symbols.empty())
            DeleteSymbolsFromRendering(mwm.first);
          else
            SendSymbolsToRendering(std::move(symbols));
        }
        else if (mwm.second == RequestType::Delete)
        {
          campaignsContainer.DeleteSection(countryName);
          expirationContainer.DeleteSection(countryName);
          DeleteSymbolsFromRendering(mwm.first);
        }
      }
      campaignsContainer.Finish();
      expirationContainer.Finish();
    }
    catch (RootException const & ex)
    {
      LOG(LWARNING, (ex.Msg()));
    }
    campaignMwms.clear();
  }
}

bool LocalAdsManager::WaitForRequest(std::vector<Request> & campaignMwms)
{
  std::unique_lock<std::mutex> lock(m_mutex);

  m_condition.wait(lock, [this] {return !m_isRunning || !m_requestedCampaigns.empty();});

  if (!m_isRunning)
    return false;

  if (!m_requestedCampaigns.empty())
    campaignMwms.swap(m_requestedCampaigns);

  return true;
}

void LocalAdsManager::OnDownloadCountry(std::string const & countryName)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_campaigns.erase(countryName);
  m_expiration.erase(countryName);
}

void LocalAdsManager::OnDeleteCountry(std::string const & countryName)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_campaigns.erase(countryName);
  m_expiration.erase(countryName);
  m_requestedCampaigns.push_back(std::make_pair(m_getMwmIdByNameFn(countryName), RequestType::Delete));
  m_condition.notify_one();
}

string LocalAdsManager::MakeRemoteURL(MwmSet::MwmId const &mwmId) const
{
  // TODO: build correct URL after server completion.

  return "http://172.27.15.68/campaigns.data";
}

std::vector<uint8_t> LocalAdsManager::DownloadCampaign(MwmSet::MwmId const & mwmId) const
{
  platform::HttpClient request(MakeRemoteURL(mwmId));
  if (!request.RunHttpRequest() || request.ErrorCode() != 200)
    return std::vector<uint8_t>();
  string const & response = request.ServerResponse();
  return std::vector<uint8_t>(response.cbegin(), response.cend());
}

df::CustomSymbols LocalAdsManager::DeserializeCampaign(std::vector<uint8_t> && rawData,
                                                  std::chrono::steady_clock::time_point timestamp)
{
  df::CustomSymbols symbols;
  // TODO: Deserialize campaign.
  // TODO: Filter by timestamp.

  // TEMP!
  //auto mwmId = m_getMwmIdByNameFn("Russia_Moscow");
  //symbols.insert(std::make_pair(FeatureID(mwmId, 371323), df::CustomSymbol("test-m", true)));
  //symbols.insert(std::make_pair(FeatureID(mwmId, 371363), df::CustomSymbol("test-m", true)));
  //symbols.insert(std::make_pair(FeatureID(mwmId, 373911), df::CustomSymbol("test-m", true)));

  return symbols;
}

void LocalAdsManager::ReadExpirationFile(std::string const & expirationFile)
{
  if (!GetPlatform().IsFileExistsByFullPath(expirationFile))
  {
    FilesContainerW f(expirationFile);
    return;
  }

  try
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    FilesContainerR expirationContainer(expirationFile);
    expirationContainer.ForEachTag([this, &expirationContainer](FilesContainerR::Tag const & tag)
    {
      m_expiration[tag] = DeserializeTimestamp(expirationContainer.GetReader(tag));
    });
  }
  catch (Reader::Exception const & ex)
  {
    LOG(LWARNING, ("Error reading file:", expirationFile, ex.Msg()));
  }
}

void LocalAdsManager::ReadCampaignFile(std::string const & campaignFile)
{
  if (!GetPlatform().IsFileExistsByFullPath(campaignFile))
  {
    FilesContainerW f(campaignFile);
    return;
  }

  try
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    df::CustomSymbols allSymbols;
    FilesContainerR campaignContainer(campaignFile);
    campaignContainer.ForEachTag([this, &campaignContainer, &allSymbols](FilesContainerR::Tag const & tag)
    {
      auto const & reader = campaignContainer.GetReader(tag);
      std::vector<uint8_t> rawData(reader.Size());
      reader.Read(0, rawData.data(), rawData.size());
      m_campaigns[tag] = false;

      ASSERT(m_expiration.find(tag) != m_expiration.end(), ());
      auto ts = m_expiration[tag];
      auto const symbols = DeserializeCampaign(std::move(rawData), ts);
      allSymbols.insert(symbols.begin(), symbols.end());
    });
    SendSymbolsToRendering(std::move(allSymbols));
  }
  catch (Reader::Exception const & ex)
  {
    LOG(LWARNING, ("Error reading file:", campaignFile, ex.Msg()));
  }
}

void LocalAdsManager::SendSymbolsToRendering(df::CustomSymbols && symbols)
{
  if (symbols.empty())
    return;

  if (m_drapeEngine == nullptr)
  {
    std::lock_guard<std::mutex> lock(m_symbolsCacheMutex);
    m_symbolsCache.insert(symbols.begin(), symbols.end());
    return;
  }
  m_drapeEngine->AddCustomSymbols(std::move(symbols));
}

void LocalAdsManager::DeleteSymbolsFromRendering(MwmSet::MwmId const & mwmId)
{
  if (m_drapeEngine != nullptr)
    m_drapeEngine->RemoveCustomSymbols(mwmId);
}
