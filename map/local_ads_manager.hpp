#pragma once

#include "map/purchase.hpp"

#include "local_ads/statistics.hpp"

#include "drape_frontend/custom_features_context.hpp"
#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "drape/pointers.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include "indexer/feature.hpp"
#include "indexer/ftypes_mapping.hpp"
#include "indexer/map_object.hpp"
#include "indexer/mwm_set.hpp"

#include "platform/location.hpp"

#include "base/thread.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

namespace feature
{
class TypesHolder;
}

class BookmarkManager;
struct LocalAdsMarkData;

struct CampaignFeature
{
  CampaignFeature() = default;
  CampaignFeature(int64_t const & mwmVersion, std::string const & countryId, uint32_t featureIndex,
                  double lat, double lon)
    : m_mwmVersion(mwmVersion), m_countryId(countryId), m_featureIndex(featureIndex), m_lat(lat), m_lon(lon) {}

  int64_t m_mwmVersion = FeatureID::kInvalidMwmVersion;
  std::string m_countryId;
  uint32_t m_featureIndex = 0;
  double m_lat = 0.0;
  double m_lon = 0.0;
};

class LocalAdsManager : public SubscriptionListener
{
public:
  using GetMwmsByRectFn = std::function<std::vector<MwmSet::MwmId>(m2::RectD const &)>;
  using GetMwmIdByNameFn = std::function<MwmSet::MwmId(std::string const &)>;
  using ReadFeatureTypeFn = std::function<void(FeatureType &)>;
  using ReadFeaturesFn = std::function<void(ReadFeatureTypeFn const &,
                                            std::vector<FeatureID> const & features)>;
  using GetMapObjectByIdFn = std::function<osm::MapObject(FeatureID const &)>;
  using OnCampaignFeaturesFn = std::function<void(std::vector<FeatureID> const & features)>;
  using Timestamp = local_ads::Timestamp;

  LocalAdsManager(GetMwmsByRectFn && getMwmsByRectFn, GetMwmIdByNameFn && getMwmIdByName,
                  ReadFeaturesFn && readFeaturesFn, GetMapObjectByIdFn && getMapObjectByIdFn);

  void Startup(BookmarkManager * bmManager, bool isEnabled);
  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);
  void UpdateViewport(ScreenBase const & screen);

  void OnDownloadCountry(std::string const & countryName);
  void OnMwmDeregistered(platform::LocalCountryFile const & countryFile);

  void Invalidate();

  local_ads::Statistics & GetStatistics() { return m_statistics; }
  local_ads::Statistics const & GetStatistics() const { return m_statistics; }
    
  bool HasAds(FeatureID const & featureId) const;
  bool HasVisualization(FeatureID const & featureId) const;
  bool IsSupportedType(feature::TypesHolder const & types) const;

  std::string GetCompanyUrl(FeatureID const & featureId) const;

  void OnSubscriptionChanged(SubscriptionType type, bool isActive) override;
  void OnLocationUpdate(location::GpsInfo const & info, int zoomLevel);

private:
  enum class RequestType
  {
    Download,
    Delete
  };
  using Request = std::pair<MwmSet::MwmId, RequestType>;

  struct CacheEntry
  {
    bool m_isVisibleOnMap = false;
    m2::PointD m_position;
  };
  using FeaturesCache = std::map<FeatureID, CacheEntry>;

  void Start();

  void InvalidateImpl();

  void ProcessRequests(std::set<Request> && campaignMwms);

  void ReadCampaignFile(std::string const & campaignFile);
  void WriteCampaignFile(std::string const & campaignFile);

  void WriteFeaturesFile(std::string const & featuresFile);

  using CampaignData = std::map<FeatureID, std::shared_ptr<LocalAdsMarkData>>;
  CampaignData ParseCampaign(std::vector<uint8_t> const & rawData, MwmSet::MwmId const & mwmId,
                             Timestamp timestamp, GetMapObjectByIdFn const & getMapObjectByIdFn) const;
  FeaturesCache ReadCampaignFeatures(CampaignData & campaignData) const;

  void CreateLocalAdsMarks(CampaignData && campaignData);
  void DeleteLocalAdsMarks(MwmSet::MwmId const & mwmId);
  void DeleteAllLocalAdsMarks();

  void UpdateFeaturesCache(FeaturesCache && features);
  void ClearLocalAdsForMwm(MwmSet::MwmId const &mwmId);

  void FillSupportedTypes();

  // Returned value means if downloading process finished correctly or was interrupted
  // by some reason.
  bool DownloadCampaign(MwmSet::MwmId const & mwmId, std::vector<uint8_t> & bytes);
  
  void RequestCampaigns(std::vector<MwmSet::MwmId> const & mwmIds);

  GetMwmsByRectFn const m_getMwmsByRectFn;
  GetMwmIdByNameFn const m_getMwmIdByNameFn;
  ReadFeaturesFn const m_readFeaturesFn;
  GetMapObjectByIdFn const m_getMapObjectByIdFn;

  std::atomic<bool> m_isStarted;

  BookmarkManager * m_bmManager;

  df::DrapeEngineSafePtr m_drapeEngine;

  std::map<std::string, bool> m_campaigns;
  struct CampaignInfo
  {
    Timestamp m_created;
    std::vector<uint8_t> m_data;
  };
  std::map<std::string, CampaignInfo> m_info;
  std::mutex m_campaignsMutex;

  FeaturesCache m_featuresCache;
  mutable std::mutex m_featuresCacheMutex;

  ftypes::HashSetMatcher<uint32_t> m_supportedTypes;

  struct BackoffStats
  {
    BackoffStats() = default;
    BackoffStats(std::chrono::steady_clock::time_point lastDownloading,
                 std::chrono::seconds currentTimeout,
                 uint8_t attemptsCount, bool fileIsAbsent)
      : m_lastDownloading(lastDownloading)
      , m_currentTimeout(currentTimeout)
      , m_attemptsCount(attemptsCount)
      , m_fileIsAbsent(fileIsAbsent)
    {}

    std::chrono::steady_clock::time_point m_lastDownloading = {};
    std::chrono::seconds m_currentTimeout = std::chrono::seconds(0);
    uint8_t m_attemptsCount = 0;
    bool m_fileIsAbsent = false;

    bool CanRetry() const;
  };
  std::map<MwmSet::MwmId, BackoffStats> m_failedDownloads;
  std::set<MwmSet::MwmId> m_downloadingMwms;
  std::vector<MwmSet::MwmId> m_lastMwms;

  local_ads::Statistics m_statistics;
  std::atomic<bool> m_isEnabled = false;

  m2::PointD m_lastCheckedPos;
  std::chrono::steady_clock::time_point m_lastCheckTime;
  FeatureID m_lastFoundFeature;
  uint32_t m_foundFeatureHitCount = 0;
};

namespace lightweight
{
class LocalAdsFeaturesReader
{
public:
  std::vector<CampaignFeature> GetCampaignFeatures(double lat, double lon,
                                                   double radiusInMeters, uint32_t maxCount);
private:
  void ReadCampaignFeaturesFile();

  bool m_initialized = false;
  std::vector<CampaignFeature> m_features;
};

class Statistics
{
public:
  void RegisterEvent(local_ads::Event && event);

private:
  local_ads::Statistics m_statistics;
};
}  // namespace lightweight
