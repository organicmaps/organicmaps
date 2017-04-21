#include "map/local_ads_manager.hpp"

#include "local_ads/campaign_serialization.hpp"
#include "local_ads/config.hpp"
#include "local_ads/file_helpers.hpp"
#include "local_ads/icons_info.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/feature_data.hpp"
#include "indexer/scales.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/url_encode.hpp"
#include "coding/zlib.hpp"

#include "3party/jansson/myjansson.hpp"

#include "private.h"

#include <cstring>
#include <sstream>

// First implementation of local ads statistics serialization
// is deflated JSON.
#define TEMPORARY_LOCAL_ADS_JSON_SERIALIZATION

namespace
{
std::string const kServerUrl = LOCAL_ADS_SERVER_URL;

std::string const kCampaignFile = "local_ads_campaigns.dat";
std::string const kLocalAdsSymbolsFile = "local_ads_symbols.txt";
auto constexpr kWWanUpdateTimeout = std::chrono::hours(12);

void SerializeCampaign(FileWriter & writer, std::string const & countryName,
                       LocalAdsManager::Timestamp const & ts,
                       std::vector<uint8_t> const & rawData)
{
  local_ads::WriteCountryName(writer, countryName);
  local_ads::WriteTimestamp<std::chrono::hours>(writer, ts);
  local_ads::WriteRawData(writer, rawData);
}

void DeserializeCampaign(ReaderSource<FileReader> & src, std::string & countryName,
                         LocalAdsManager::Timestamp & ts, std::vector<uint8_t> & rawData)
{
  countryName = local_ads::ReadCountryName(src);
  ts = local_ads::ReadTimestamp<std::chrono::hours>(src);
  rawData = local_ads::ReadRawData(src);
}

std::string GetPath(std::string const & fileName)
{
  return my::JoinFoldersToPath(GetPlatform().WritableDir(), fileName);
}

std::chrono::steady_clock::time_point Now()
{
  return std::chrono::steady_clock::now();
}

std::string MakeRemoteURL(MwmSet::MwmId const & mwmId)
{
  if (kServerUrl.empty() || !mwmId.IsAlive())
    return {};

  std::ostringstream ss;
  ss << kServerUrl << "/";
  ss << mwmId.GetInfo()->GetVersion() << "/";
  ss << UrlEncode(mwmId.GetInfo()->GetCountryName()) << ".ads";
  return ss.str();
}

std::vector<uint8_t> DownloadCampaign(MwmSet::MwmId const & mwmId)
{
  std::string const url = MakeRemoteURL(mwmId);
  if (url.empty())
    return {};

  platform::HttpClient request(url);
  if (!request.RunHttpRequest() || request.ErrorCode() != 200)
    return {};

  string const & response = request.ServerResponse();
  return std::vector<uint8_t>(response.cbegin(), response.cend());
}

df::CustomSymbols ParseCampaign(std::vector<uint8_t> const & rawData, MwmSet::MwmId const & mwmId,
                                LocalAdsManager::Timestamp timestamp)
{
  df::CustomSymbols symbols;

  auto campaigns = local_ads::Deserialize(rawData);
  for (local_ads::Campaign const & campaign : campaigns)
  {
    if (Now() > timestamp + std::chrono::hours(24 * campaign.m_daysBeforeExpired))
      continue;
    symbols.insert(
        std::make_pair(FeatureID(mwmId, campaign.m_featureId),
                       df::CustomSymbol(campaign.GetIconName(), campaign.m_priorityBit)));
  }

  return symbols;
}

#ifdef TEMPORARY_LOCAL_ADS_JSON_SERIALIZATION
std::vector<uint8_t> SerializeLocalAdsToJSON(std::list<local_ads::Event> const & events,
                                             std::string const & userId, std::string & contentType,
                                             std::string & contentEncoding)
{
  using namespace std::chrono;
  ASSERT(!events.empty(), ());
  auto root = my::NewJSONObject();
  ToJSONObject(*root, "userId", userId);
  ToJSONObject(*root, "countryId", events.front().m_countryId);
  ToJSONObject(*root, "mwmVersion", events.front().m_mwmVersion);
  auto eventsNode = my::NewJSONArray();
  for (auto const & event : events)
  {
    auto eventNode = my::NewJSONObject();
    auto s = duration_cast<seconds>(event.m_timestamp.time_since_epoch()).count();
    ToJSONObject(*eventNode, "type", static_cast<uint8_t>(event.m_type));
    ToJSONObject(*eventNode, "timestamp", static_cast<int64_t>(s));
    ToJSONObject(*eventNode, "featureId", static_cast<int32_t>(event.m_featureId));
    ToJSONObject(*eventNode, "zoomLevel", event.m_zoomLevel);
    ToJSONObject(*eventNode, "latitude", event.m_latitude);
    ToJSONObject(*eventNode, "longitude", event.m_longitude);
    ToJSONObject(*eventNode, "accuracyInMeters", event.m_accuracyInMeters);
    json_array_append_new(eventsNode.get(), eventNode.release());
  }
  json_object_set_new(root.get(), "events", eventsNode.release());
  std::unique_ptr<char, JSONFreeDeleter> buffer(
      json_dumps(root.get(), JSON_COMPACT | JSON_ENSURE_ASCII));
  std::vector<uint8_t> result;
  coding::ZLib::Deflate(buffer.get(), strlen(buffer.get()), coding::ZLib::Level::BestCompression,
                        std::back_inserter(result));
  return result;
}
#endif
}  // namespace

LocalAdsManager::LocalAdsManager(GetMwmsByRectFn const & getMwmsByRectFn,
                       GetMwmIdByName const & getMwmIdByName)
  : m_getMwmsByRectFn(getMwmsByRectFn)
  , m_getMwmIdByNameFn(getMwmIdByName)
{
  CHECK(m_getMwmsByRectFn != nullptr, ());
  CHECK(m_getMwmIdByNameFn != nullptr, ());

  m_statistics.SetUserId(GetPlatform().UniqueClientId());

#ifdef TEMPORARY_LOCAL_ADS_JSON_SERIALIZATION
  using namespace std::placeholders;
  m_statistics.SetCustomServerSerializer(std::bind(&SerializeLocalAdsToJSON, _1, _2, _3, _4));
#endif
}

LocalAdsManager::~LocalAdsManager()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  ASSERT(!m_isRunning, ());
}

void LocalAdsManager::Startup()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isRunning)
      return;
    m_isRunning = true;
  }
  FillSupportedTypes();

  m_thread = threads::SimpleThread(&LocalAdsManager::ThreadRoutine, this);

  m_statistics.Startup();
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

  m_statistics.Teardown();
}

void LocalAdsManager::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  m_drapeEngine = engine;
  {
    std::lock_guard<std::mutex> lock(m_symbolsCacheMutex);
    if (m_symbolsCache.empty())
      return;

    m_drapeEngine->AddCustomSymbols(std::move(m_symbolsCache));
    m_symbolsCache.clear();
  }
}

void LocalAdsManager::UpdateViewport(ScreenBase const & screen)
{
  auto connectionStatus = GetPlatform().ConnectionStatus();
  if (kServerUrl.empty() || connectionStatus == Platform::EConnectionType::CONNECTION_NONE ||
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
    if (!m_isRunning)
      return;

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
        auto const it = m_info.find(mwmName);
        bool needUpdateByTimeout = (connectionStatus == Platform::EConnectionType::CONNECTION_WIFI);
        if (!needUpdateByTimeout && it != m_info.end())
          needUpdateByTimeout = Now() > (it->second.m_created + kWWanUpdateTimeout);

        if (needUpdateByTimeout || it == m_info.end())
          requestedCampaigns.push_back(mwmName);
      }
    }

    if (!requestedCampaigns.empty())
    {
      for (auto const & campaign : requestedCampaigns)
        m_requestedCampaigns.insert(std::make_pair(m_getMwmIdByNameFn(campaign), RequestType::Download));
      m_condition.notify_one();
    }
  }
}

void LocalAdsManager::ThreadRoutine()
{
  local_ads::IconsInfo::Instance().SetSourceFile(kLocalAdsSymbolsFile);

  std::string const campaignFile = GetPath(kCampaignFile);

  // Read persistence data.
  ReadCampaignFile(campaignFile);
  Invalidate();

  std::set<Request> campaignMwms;
  while (WaitForRequest(campaignMwms))
  {
    for (auto const & mwm : campaignMwms)
    {
      if (!mwm.first.IsAlive())
        continue;

      std::string const countryName = mwm.first.GetInfo()->GetCountryName();
      if (mwm.second == RequestType::Download)
      {
        // Download campaign data from server.
        CampaignInfo info;
        info.m_data = DownloadCampaign(mwm.first);
        if (info.m_data.empty())
          continue;
        info.m_created = Now();

        // Parse data and send symbols to rendering.
        auto symbols = ParseCampaign(std::move(info.m_data), mwm.first, info.m_created);
        if (symbols.empty())
        {
          std::lock_guard<std::mutex> lock(m_mutex);
          m_campaigns.erase(countryName);
          m_info.erase(countryName);
          DeleteSymbolsFromRendering(mwm.first);
        }
        else
        {
          UpdateFeaturesCache(symbols);
          SendSymbolsToRendering(std::move(symbols));
        }

        // Update run-time data.
        {
          std::lock_guard<std::mutex> lock(m_mutex);
          m_campaigns[countryName] = true;
          m_info[countryName] = info;
        }
      }
      else if (mwm.second == RequestType::Delete)
      {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_campaigns.erase(countryName);
        m_info.erase(countryName);
        DeleteSymbolsFromRendering(mwm.first);
      }
    }
    campaignMwms.clear();

    // Save data persistently.
    WriteCampaignFile(campaignFile);
  }
}

bool LocalAdsManager::WaitForRequest(std::set<Request> & campaignMwms)
{
  std::unique_lock<std::mutex> lock(m_mutex);

  m_condition.wait(lock, [this] {return !m_isRunning || !m_requestedCampaigns.empty();});
  if (!m_isRunning)
    return false;

  campaignMwms = std::move(m_requestedCampaigns);
  m_requestedCampaigns.clear();
  return true;
}

void LocalAdsManager::OnDownloadCountry(std::string const & countryName)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_campaigns.erase(countryName);
  m_info.erase(countryName);
}

void LocalAdsManager::OnDeleteCountry(std::string const & countryName)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_requestedCampaigns.insert(std::make_pair(m_getMwmIdByNameFn(countryName), RequestType::Delete));
  m_condition.notify_one();
}

void LocalAdsManager::ReadCampaignFile(std::string const & campaignFile)
{
  if (!GetPlatform().IsFileExistsByFullPath(campaignFile))
    return;

  std::lock_guard<std::mutex> lock(m_mutex);
  try
  {
    FileReader reader(campaignFile, true /* withExceptions */);
    ReaderSource<FileReader> src(reader);
    while (src.Size() > 0)
    {
      std::string countryName;
      CampaignInfo info;
      DeserializeCampaign(src, countryName, info.m_created, info.m_data);
      m_info[countryName] = info;
      m_campaigns[countryName] = false;
    }
  }
  catch (Reader::Exception const & ex)
  {
    LOG(LWARNING, ("Error reading file:", campaignFile, ex.Msg()));
    FileWriter::DeleteFileX(campaignFile);
    m_info.clear();
    m_campaigns.clear();
  }
}

void LocalAdsManager::WriteCampaignFile(std::string const & campaignFile)
{
  try
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    FileWriter writer(campaignFile);
    for (auto const & info : m_info)
      SerializeCampaign(writer, info.first, info.second.m_created, info.second.m_data);
  }
  catch (RootException const & ex)
  {
    LOG(LWARNING, ("Error writing file:", campaignFile, ex.Msg()));
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

void LocalAdsManager::Invalidate()
{
  if (m_drapeEngine != nullptr)
    m_drapeEngine->RemoveAllCustomSymbols();

  df::CustomSymbols symbols;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto const & info : m_info)
    {
      auto campaignSymbols = ParseCampaign(info.second.m_data, m_getMwmIdByNameFn(info.first),
                                           info.second.m_created);
      symbols.insert(campaignSymbols.begin(), campaignSymbols.end());
    }
  }
  UpdateFeaturesCache(symbols);
  SendSymbolsToRendering(std::move(symbols));
}

void LocalAdsManager::UpdateFeaturesCache(df::CustomSymbols const & symbols)
{
  if (symbols.empty())
    return;

  std::lock_guard<std::mutex> lock(m_featuresCacheMutex);
  for (auto const & symbolPair : symbols)
    m_featuresCache.insert(symbolPair.first);
}

bool LocalAdsManager::Contains(FeatureID const & featureId) const
{
  std::lock_guard<std::mutex> lock(m_featuresCacheMutex);
  return m_featuresCache.find(featureId) != m_featuresCache.cend();
}

bool LocalAdsManager::IsSupportedType(feature::TypesHolder const & types) const
{
  return m_supportedTypes.Contains(types);
}

std::string LocalAdsManager::GetStartCompanyUrl() const
{
  return LOCAL_ADS_START_COMPANY_PAGE_HOST;
}

std::string LocalAdsManager::GetShowStatisticUrl() const { return LOCAL_ADS_STATISTICS_PAGE_HOST; }
