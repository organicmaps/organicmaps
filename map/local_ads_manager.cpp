#include "map/local_ads_manager.hpp"
#include "map/bookmark_manager.hpp"
#include "map/local_ads_mark.hpp"

#include "local_ads/campaign_serialization.hpp"
#include "local_ads/config.hpp"
#include "local_ads/file_helpers.hpp"
#include "local_ads/icons_info.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/feature_algo.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/scales.hpp"

#include "platform/http_client.hpp"
#include "platform/marketing_service.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/multilang_utf8_string.hpp"
#include "coding/url_encode.hpp"

#include "base/url_helpers.hpp"

#include "private.h"

#include <array>
#include <cstring>
#include <sstream>

using namespace base;

namespace
{
std::array<char const * const, 5> const kMarketingParameters = {{marketing::kFrom, marketing::kType, marketing::kName,
                                                                 marketing::kContent, marketing::kKeyword}};
std::string const kServerUrl = LOCAL_ADS_SERVER_URL;
std::string const kCampaignPageUrl = LOCAL_ADS_COMPANY_PAGE_URL;

std::string const kCampaignFile = "local_ads_campaigns.dat";
std::string const kLocalAdsSymbolsFile = "local_ads_symbols.txt";
auto constexpr kWWanUpdateTimeout = std::chrono::hours(12);
uint8_t constexpr kRequestMinZoomLevel = 12;
auto constexpr kFailedDownloadingTimeout = std::chrono::seconds(2);
auto constexpr kMaxDownloadingAttempts = 5;

auto constexpr kHiddenFeaturePriority = 1;

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

std::string MakeCampaignDownloadingURL(MwmSet::MwmId const & mwmId)
{
  if (kServerUrl.empty() || !mwmId.IsAlive())
    return {};

  std::ostringstream ss;
  auto const campaignDataVersion = static_cast<uint32_t>(local_ads::Version::Latest);
  ss << kServerUrl << "/"
     << campaignDataVersion << "/"
     << mwmId.GetInfo()->GetVersion() << "/"
     << UrlEncode(mwmId.GetInfo()->GetCountryName()) << ".ads";
  return ss.str();
}

std::string GetCustomIcon(FeatureType & featureType)
{
  auto const & metadata = featureType.GetMetadata();
  auto const websiteStr = metadata.Get(feature::Metadata::FMD_WEBSITE);
  if (websiteStr.find("burgerking") != std::string::npos)
    return "0_burger-king";

  auto const bannerUrl = metadata.Get(feature::Metadata::FMD_BANNER_URL);
  if (bannerUrl.find("mcarthurglen") != std::string::npos)
    return "partner1-l";

  if (bannerUrl.find("adidas") != std::string::npos)
  {
    if (bannerUrl.find("originals") != std::string::npos)
      return "partner6-l";
    if (bannerUrl.find("deti") != std::string::npos)
      return "partner7-l";
    return "partner4-l";
  }

  if (websiteStr.find("costacoffee") != std::string::npos ||
      bannerUrl.find("costa_coffee") != std::string::npos)
  {
    return "partner8-l";
  }

  if (websiteStr.find("tgifridays") != std::string::npos ||
      bannerUrl.find("tgi_fridays") != std::string::npos)
  {
    return "partner9-l";
  }

  if (websiteStr.find("sportmaster") != std::string::npos ||
      bannerUrl.find("sportmaster") != std::string::npos)
  {
    return "partner10-l";
  }

  if (bannerUrl.find("azbuka_vkusa") != std::string::npos)
    return "partner12-l";

  return {};
}

using CampaignData = std::map<FeatureID, std::shared_ptr<LocalAdsMarkData>>;

CampaignData ParseCampaign(std::vector<uint8_t> const & rawData, MwmSet::MwmId const & mwmId,
                           LocalAdsManager::Timestamp timestamp,
                           LocalAdsManager::GetFeatureByIdFn const & getFeatureByIdFn)
{
  ASSERT(getFeatureByIdFn != nullptr, ());
  CampaignData data;
  auto campaigns = local_ads::Deserialize(rawData);
  for (local_ads::Campaign const & campaign : campaigns)
  {
    FeatureID featureId(mwmId, campaign.m_featureId);
    if (!featureId.IsValid())
      continue;

    if (campaign.m_priority == kHiddenFeaturePriority)
    {
      data.insert(std::make_pair(featureId, nullptr));
      continue;
    }

    std::string iconName = campaign.GetIconName();
    auto const expiration = timestamp + std::chrono::hours(24 * campaign.m_daysBeforeExpired);
    if (iconName.empty() || local_ads::Clock::now() > expiration)
      continue;

    FeatureType featureType;
    if (getFeatureByIdFn(featureId, featureType))
    {
      auto const customIcon = GetCustomIcon(featureType);
      if (!customIcon.empty())
        iconName = customIcon;
    }

    auto markData = std::make_shared<LocalAdsMarkData>();
    markData->m_symbolName = iconName;
    markData->m_minZoomLevel = campaign.m_minZoomLevel;
    data.insert(std::make_pair(featureId, std::move(markData)));
  }

  return data;
}

df::CustomFeatures ReadCampaignFeatures(LocalAdsManager::ReadFeaturesFn const & reader,
                                        CampaignData & campaignData)
{
  ASSERT(reader != nullptr, ());

  std::vector<FeatureID> features;
  features.reserve(campaignData.size());
  df::CustomFeatures customFeatures;
  for (auto const & data : campaignData)
  {
    if (data.second)
      features.push_back(data.first);
    customFeatures.insert(std::make_pair(data.first, data.second != nullptr));
  }

  auto const deviceLang = StringUtf8Multilang::GetLangIndex(languages::GetCurrentNorm());
  reader(
      [&campaignData, deviceLang](FeatureType & ft) {
        auto it = campaignData.find(ft.GetID());
        CHECK(it != campaignData.end(), ());
        CHECK(it->second != nullptr, ());
        it->second->m_position = feature::GetCenter(ft, scales::GetUpperScale());
        ft.GetPreferredNames(true /* allowTranslit */, deviceLang, it->second->m_mainText,
                             it->second->m_auxText);
      },
      features);

  return customFeatures;
}

void CreateLocalAdsMarks(BookmarkManager * bmManager, CampaignData && campaignData)
{
  if (bmManager == nullptr)
    return;

  // Here we copy campaign data, because we can create user marks only from UI thread.
  GetPlatform().RunTask(Platform::Thread::Gui, [bmManager, campaignData = std::move(campaignData)]()
  {
    auto editSession = bmManager->GetEditSession();
    for (auto const & data : campaignData)
    {
      if (data.second == nullptr)
        continue;

      auto * mark = editSession.CreateUserMark<LocalAdsMark>(data.second->m_position);
      mark->SetData(LocalAdsMarkData(*data.second));
      mark->SetFeatureId(data.first);
    }
  });
}

void DeleteLocalAdsMarks(BookmarkManager * bmManager, MwmSet::MwmId const & mwmId)
{
  if (bmManager == nullptr)
    return;

  GetPlatform().RunTask(Platform::Thread::Gui, [bmManager, mwmId]()
  {
    bmManager->GetEditSession().DeleteUserMarks<LocalAdsMark>(UserMark::Type::LOCAL_ADS,
                                                              [&mwmId](LocalAdsMark const * mark)
                                                              {
                                                                return mark->GetFeatureID().m_mwmId == mwmId;
                                                              });
  });
}

void DeleteAllLocalAdsMarks(BookmarkManager * bmManager)
{
  if (bmManager == nullptr)
    return;

  GetPlatform().RunTask(Platform::Thread::Gui, [bmManager]()
  {
    bmManager->GetEditSession().ClearGroup(UserMark::Type::LOCAL_ADS);
  });
}

std::string MakeCampaignPageURL(FeatureID const & featureId)
{
  if (kCampaignPageUrl.empty() || !featureId.m_mwmId.IsAlive())
    return {};

  std::ostringstream ss;
  ss << kCampaignPageUrl << "/" << featureId.m_mwmId.GetInfo()->GetVersion() << "/"
     << UrlEncode(featureId.m_mwmId.GetInfo()->GetCountryName()) << "/" << featureId.m_index;

  url::Params params;
  params.reserve(kMarketingParameters.size());
  for (auto const & key : kMarketingParameters)
  {
    string value;
    if (!marketing::Settings::Get(key, value))
      continue;

    params.push_back({key, value});
  }

  return url::Make(ss.str(), params);
}
}  // namespace

LocalAdsManager::LocalAdsManager(GetMwmsByRectFn && getMwmsByRectFn,
                                 GetMwmIdByNameFn && getMwmIdByName,
                                 ReadFeaturesFn && readFeaturesFn,
                                 GetFeatureByIdFn && getFeatureByIDFn)
  : m_getMwmsByRectFn(std::move(getMwmsByRectFn))
  , m_getMwmIdByNameFn(std::move(getMwmIdByName))
  , m_readFeaturesFn(std::move(readFeaturesFn))
  , m_getFeatureByIdFn(std::move(getFeatureByIDFn))
  , m_isStarted(false)
  , m_bmManager(nullptr)
{
  CHECK(m_getMwmsByRectFn != nullptr, ());
  CHECK(m_getMwmIdByNameFn != nullptr, ());
  CHECK(m_readFeaturesFn != nullptr, ());
  CHECK(m_getFeatureByIdFn != nullptr, ());
}

void LocalAdsManager::Startup(BookmarkManager * bmManager, bool isEnabled)
{
  m_isEnabled = isEnabled;
  FillSupportedTypes();
  m_bmManager = bmManager;

  if (isEnabled)
    Start();
}

void LocalAdsManager::Start()
{
  m_isStarted = true;
  GetPlatform().RunTask(Platform::Thread::File, [this]
  {
    local_ads::IconsInfo::Instance().SetSourceFile(kLocalAdsSymbolsFile);

    std::string const campaignFile = GetPath(kCampaignFile);

    // Read persistence data.
    ReadCampaignFile(campaignFile);
    Invalidate();
  });

  m_statistics.Startup();
}

void LocalAdsManager::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  m_drapeEngine.Set(engine);
  UpdateFeaturesCache({});
}

void LocalAdsManager::UpdateViewport(ScreenBase const & screen)
{
  auto const connectionStatus = GetPlatform().ConnectionStatus();
  if (!m_isStarted || !m_isEnabled || kServerUrl.empty() ||
      connectionStatus == Platform::EConnectionType::CONNECTION_NONE ||
      df::GetZoomLevel(screen.GetScale()) < kRequestMinZoomLevel)
  {
    return;
  }

  // This function must be called on UI-thread.
  auto mwms = m_getMwmsByRectFn(screen.ClipRect());
  if (mwms.empty())
    return;

  // Request local ads campaigns.
  GetPlatform().RunTask(Platform::Thread::File, [this, mwms = std::move(mwms)]()
  {
    RequestCampaigns(mwms);
  });
}

void LocalAdsManager::RequestCampaigns(std::vector<MwmSet::MwmId> const & mwmIds)
{
  auto const connectionStatus = GetPlatform().ConnectionStatus();
  
  std::vector<MwmSet::MwmId> requestedCampaigns;
  {
    std::lock_guard<std::mutex> lock(m_campaignsMutex);
    for (auto const & mwm : mwmIds)
    {
      auto info = mwm.GetInfo();
      if (!info)
        continue;
      
      // Skip currently downloading mwms.
      if (m_downloadingMwms.find(mwm) != m_downloadingMwms.cend())
        continue;

      // Skip downloading request if maximum attempts count has been reached or
      // we are waiting for new attempt.
      auto const failedDownloadsIt = m_failedDownloads.find(mwm);
      if (failedDownloadsIt != m_failedDownloads.cend() && !failedDownloadsIt->second.CanRetry())
        continue;
      
      std::string const & mwmName = info->GetCountryName();
      auto campaignIt = m_campaigns.find(mwmName);
      if (campaignIt == m_campaigns.end())
      {
        requestedCampaigns.push_back(mwm);
        m_downloadingMwms.insert(mwm);
        continue;
      }
      
      // If a campaign has not been requested from server this session.
      if (!campaignIt->second)
      {
        auto const it = m_info.find(mwmName);
        bool needUpdateByTimeout = (connectionStatus == Platform::EConnectionType::CONNECTION_WIFI);
        if (!needUpdateByTimeout && it != m_info.end())
          needUpdateByTimeout = local_ads::Clock::now() > (it->second.m_created + kWWanUpdateTimeout);
        
        if (needUpdateByTimeout || it == m_info.end())
        {
          requestedCampaigns.push_back(mwm);
          m_downloadingMwms.insert(mwm);
        }
      }
    }
  }
  
  if (requestedCampaigns.empty())
    return;
  
  std::set<Request> requests;
  for (auto const & campaign : requestedCampaigns)
    requests.insert(std::make_pair(campaign, RequestType::Download));
  ProcessRequests(std::move(requests));
}

bool LocalAdsManager::DownloadCampaign(MwmSet::MwmId const & mwmId, std::vector<uint8_t> & bytes)
{
  bytes.clear();

  std::string const url = MakeCampaignDownloadingURL(mwmId);
  if (url.empty())
    return true; // In this case empty result is valid.

  // Skip already downloaded campaigns.
  auto const & countryName = mwmId.GetInfo()->GetCountryName();
  {
    std::lock_guard<std::mutex> lock(m_campaignsMutex);
    auto const it = m_campaigns.find(countryName);
    if (it != m_campaigns.cend() && it->second)
      return false;
  }

  platform::HttpClient request(url);
  request.SetTimeout(5); // timeout in seconds
  request.SetRawHeader("User-Agent", GetPlatform().GetAppUserAgent());
  bool const success = request.RunHttpRequest() && request.ErrorCode() == 200;

  std::lock_guard<std::mutex> lock(m_campaignsMutex);
  m_downloadingMwms.erase(mwmId);
  if (!success)
  {
    bool const isAbsent = request.ErrorCode() == 404;
    auto const it = m_failedDownloads.find(mwmId);
    if (it == m_failedDownloads.cend())
    {
      m_failedDownloads.insert(std::make_pair(mwmId, BackoffStats(std::chrono::steady_clock::now(),
                                                                  kFailedDownloadingTimeout,
                                                                  1 /* m_attemptsCount */, isAbsent)));
    }
    else
    {
      // Here we increase timeout multiplying by 2.
      it->second.m_currentTimeout = std::chrono::seconds(it->second.m_currentTimeout.count() * 2);
      it->second.m_attemptsCount++;
      it->second.m_fileIsAbsent = isAbsent;
    }
    return false;
  }

  m_failedDownloads.erase(mwmId);
  auto const & response = request.ServerResponse();
  bytes.assign(response.cbegin(), response.cend());
  return true;
}

void LocalAdsManager::ProcessRequests(std::set<Request> && requests)
{
  GetPlatform().RunTask(Platform::Thread::Network, [this, requests = std::move(requests)]
  {
    std::string const campaignFile = GetPath(kCampaignFile);
    for (auto const & request : requests)
    {
      auto const & mwm = request.first;
      auto const & type = request.second;
      
      if (!mwm.GetInfo())
        continue;
      
      std::string const & countryName = mwm.GetInfo()->GetCountryName();
      if (type == RequestType::Download)
      {
        // Download campaign data from server.
        CampaignInfo info;
        info.m_created = local_ads::Clock::now();
        if (!DownloadCampaign(mwm, info.m_data))
          continue;
        
        // Parse data and recreate marks.
        ClearLocalAdsForMwm(mwm);
        if (!info.m_data.empty())
        {
          auto campaignData = ParseCampaign(info.m_data, mwm, info.m_created, m_getFeatureByIdFn);
          if (!campaignData.empty())
          {
            UpdateFeaturesCache(ReadCampaignFeatures(m_readFeaturesFn, campaignData));
            CreateLocalAdsMarks(m_bmManager, std::move(campaignData));
          }
        }
        
        std::lock_guard<std::mutex> lock(m_campaignsMutex);
        m_campaigns[countryName] = true;
        m_info[countryName] = info;
      }
      else if (type == RequestType::Delete)
      {
        std::lock_guard<std::mutex> lock(m_campaignsMutex);
        m_campaigns.erase(countryName);
        m_info.erase(countryName);
        ClearLocalAdsForMwm(mwm);
      }
    }
    
    // Save data persistently.
    GetPlatform().RunTask(Platform::Thread::File, [this, campaignFile]
    {
      WriteCampaignFile(campaignFile);
    });
  });
}

void LocalAdsManager::OnDownloadCountry(std::string const & countryName)
{
  GetPlatform().RunTask(Platform::Thread::File, [this, countryName]
  {
    std::lock_guard<std::mutex> lock(m_campaignsMutex);
    m_campaigns.erase(countryName);
    m_info.erase(countryName);
  });
}

void LocalAdsManager::OnMwmDeregistered(platform::LocalCountryFile const & countryFile)
{
  MwmSet::MwmId mwmId;
  {
    std::lock_guard<std::mutex> lock(m_featuresCacheMutex);
    for (auto const & cachedMwm : m_featuresCache)
    {
      if (cachedMwm.first.m_mwmId.IsDeregistered(countryFile))
      {
        mwmId = cachedMwm.first.m_mwmId;
        break;
      }
    }
  }
  if (!mwmId.GetInfo())
    return;
  GetPlatform().RunTask(Platform::Thread::File, [this, mwmId]
  {
    ProcessRequests({std::make_pair(mwmId, RequestType::Delete)});
  });
}

void LocalAdsManager::ReadCampaignFile(std::string const & campaignFile)
{
  if (!GetPlatform().IsFileExistsByFullPath(campaignFile))
    return;

  std::lock_guard<std::mutex> lock(m_campaignsMutex);
  try
  {
    FileReader reader(campaignFile);
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
  std::lock_guard<std::mutex> lock(m_campaignsMutex);
  try
  {
    FileWriter writer(campaignFile);
    for (auto const & info : m_info)
      SerializeCampaign(writer, info.first, info.second.m_created, info.second.m_data);
  }
  catch (RootException const & ex)
  {
    LOG(LWARNING, ("Error writing file:", campaignFile, ex.Msg()));
  }
}

void LocalAdsManager::Invalidate()
{
  GetPlatform().RunTask(Platform::Thread::File, [this]
  {
    DeleteAllLocalAdsMarks(m_bmManager);
    m_drapeEngine.SafeCall(&df::DrapeEngine::RemoveAllCustomFeatures);

    CampaignData campaignData;
    {
      std::lock_guard<std::mutex> lock(m_campaignsMutex);
      for (auto const & info : m_info)
      {
        auto data = ParseCampaign(info.second.m_data, m_getMwmIdByNameFn(info.first),
                                  info.second.m_created, m_getFeatureByIdFn);
        campaignData.insert(data.begin(), data.end());
      }
    }
    UpdateFeaturesCache(ReadCampaignFeatures(m_readFeaturesFn, campaignData));
    CreateLocalAdsMarks(m_bmManager, std::move(campaignData));
  });
}

void LocalAdsManager::UpdateFeaturesCache(df::CustomFeatures && features)
{
  df::CustomFeatures featuresCache;
  {
    std::lock_guard<std::mutex> lock(m_featuresCacheMutex);
    if (!features.empty())
      m_featuresCache.insert(features.begin(), features.end());
    featuresCache = m_featuresCache;
  }
  m_drapeEngine.SafeCall(&df::DrapeEngine::SetCustomFeatures, std::move(featuresCache));
}

void LocalAdsManager::ClearLocalAdsForMwm(MwmSet::MwmId const & mwmId)
{
  // Clear feature cache.
  {
    std::lock_guard<std::mutex> lock(m_featuresCacheMutex);
    for (auto it = m_featuresCache.begin(); it != m_featuresCache.end();)
    {
      if (it->first.m_mwmId == mwmId)
        it = m_featuresCache.erase(it);
      else
        ++it;
    }
  }

  // Remove custom features in graphics engine.
  m_drapeEngine.SafeCall(&df::DrapeEngine::RemoveCustomFeatures, mwmId);

  // Delete marks.
  DeleteLocalAdsMarks(m_bmManager, mwmId);
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

std::string LocalAdsManager::GetCompanyUrl(FeatureID const & featureId) const
{
  return MakeCampaignPageURL(featureId);
}

void LocalAdsManager::OnSubscriptionChanged(SubscriptionType type, bool isActive)
{
  if (type != SubscriptionType::RemoveAds)
    return;

  bool enabled = !isActive;
  if (m_isEnabled == enabled)
    return;

  m_isEnabled = enabled;
  if (enabled)
  {
    if (!m_isStarted)
      Start();
    else
      Invalidate();
  }
  else
  {
    GetPlatform().RunTask(Platform::Thread::File, [this]
    {
      // Clear campaigns data.
      {
        std::lock_guard<std::mutex> lock(m_campaignsMutex);
        m_campaigns.clear();
        m_info.clear();
        m_failedDownloads.clear();
        m_downloadingMwms.clear();
      }

      // Clear features cache.
      {
        std::lock_guard<std::mutex> lock(m_featuresCacheMutex);
        m_featuresCache.clear();
      }

      // Clear all graphics.
      DeleteAllLocalAdsMarks(m_bmManager);
      m_drapeEngine.SafeCall(&df::DrapeEngine::RemoveAllCustomFeatures);
    });

    // Disable statistics collection.
    m_statistics.SetEnabled(false);
  }
}

bool LocalAdsManager::BackoffStats::CanRetry() const
{
  return !m_fileIsAbsent && m_attemptsCount < kMaxDownloadingAttempts &&
         std::chrono::steady_clock::now() > (m_lastDownloading + m_currentTimeout);
}
