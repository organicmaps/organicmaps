#pragma once

#include "local_ads/statistics.hpp"

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "drape/pointers.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include "indexer/ftypes_mapping.hpp"
#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"

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

class LocalAdsManager final
{
public:
  using GetMwmsByRectFn = std::function<std::vector<MwmSet::MwmId>(m2::RectD const &)>;
  using GetMwmIdByNameFn = std::function<MwmSet::MwmId(std::string const &)>;
  using ReadFeatureTypeFn = std::function<void(FeatureType const &)>;
  using ReadFeaturesFn = std::function<void(ReadFeatureTypeFn const &,
                                            std::set<FeatureID> const & features)>;
  using Timestamp = local_ads::Timestamp;

  LocalAdsManager(GetMwmsByRectFn && getMwmsByRectFn, GetMwmIdByNameFn && getMwmIdByName,
                  ReadFeaturesFn && readFeaturesFn);
  LocalAdsManager(LocalAdsManager && /* localAdsManager */) = default;

  void Startup(BookmarkManager * bmManager);
  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);
  void UpdateViewport(ScreenBase const & screen);

  void OnDownloadCountry(std::string const & countryName);
  void OnDeleteCountry(std::string const & countryName);

  void Invalidate();

  local_ads::Statistics & GetStatistics() { return m_statistics; }
  local_ads::Statistics const & GetStatistics() const { return m_statistics; }
    
  bool Contains(FeatureID const & featureId) const;
  bool IsSupportedType(feature::TypesHolder const & types) const;

  std::string GetCompanyUrl(FeatureID const & featureId) const;

private:
  enum class RequestType
  {
    Download,
    Delete
  };
  using Request = std::pair<MwmSet::MwmId, RequestType>;

  void ProcessRequests(std::set<Request> && campaignMwms);

  void ReadCampaignFile(std::string const & campaignFile);
  void WriteCampaignFile(std::string const & campaignFile);

  void UpdateFeaturesCache(std::set<FeatureID> && ids);
  void ClearLocalAdsForMwm(MwmSet::MwmId const &mwmId);

  void FillSupportedTypes();

  // Returned value means if downloading process finished correctly or was interrupted
  // by some reason.
  bool DownloadCampaign(MwmSet::MwmId const & mwmId, std::vector<uint8_t> & bytes);
  
  std::vector<std::string> GetRequestedCampaigns(std::vector<MwmSet::MwmId> && mwmIds) const;

  GetMwmsByRectFn const m_getMwmsByRectFn;
  GetMwmIdByNameFn const m_getMwmIdByNameFn;
  ReadFeaturesFn const m_readFeaturesFn;

  std::atomic<BookmarkManager *> m_bmManager;

  df::DrapeEngineSafePtr m_drapeEngine;

  std::map<std::string, bool> m_campaigns;
  struct CampaignInfo
  {
    Timestamp m_created;
    std::vector<uint8_t> m_data;
  };
  std::map<std::string, CampaignInfo> m_info;
  mutable std::mutex m_campaignsMutex;

  std::set<FeatureID> m_featuresCache;
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

  local_ads::Statistics m_statistics;
};
