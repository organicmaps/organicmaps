#pragma once

#include "local_ads/statistics.hpp"

#include "drape_frontend/custom_symbol.hpp"

#include "drape/pointers.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include "indexer/ftypes_mapping.hpp"
#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"

#include "base/thread.hpp"

#include <chrono>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

namespace df
{
class DrapeEngine;
}

namespace feature
{
class TypesHolder;
}

class LocalAdsManager final
{
public:
  using GetMwmsByRectFn = function<std::vector<MwmSet::MwmId>(m2::RectD const &)>;
  using GetMwmIdByName = function<MwmSet::MwmId(std::string const &)>;
  using Timestamp = local_ads::Timestamp;

  LocalAdsManager(GetMwmsByRectFn const & getMwmsByRectFn, GetMwmIdByName const & getMwmIdByName);
  LocalAdsManager(LocalAdsManager && /* localAdsManager */) = default;
  ~LocalAdsManager();

  void Startup();
  void Teardown();
  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);
  void UpdateViewport(ScreenBase const & screen);

  void OnDownloadCountry(std::string const & countryName);
  void OnDeleteCountry(std::string const & countryName);

  void Invalidate();

  local_ads::Statistics & GetStatistics() { return m_statistics; }
  local_ads::Statistics const & GetStatistics() const { return m_statistics; }
    
  bool Contains(FeatureID const & featureId) const;
  bool IsSupportedType(feature::TypesHolder const & types) const;

  std::string GetStartCompanyUrl() const;
  std::string GetShowStatisticUrl() const;

private:
  enum class RequestType
  {
    Download,
    Delete
  };
  using Request = std::pair<MwmSet::MwmId, RequestType>;

  void ThreadRoutine();
  bool WaitForRequest(std::set<Request> & campaignMwms);

  void SendSymbolsToRendering(df::CustomSymbols && symbols);
  void DeleteSymbolsFromRendering(MwmSet::MwmId const & mwmId);

  void ReadCampaignFile(std::string const & campaignFile);
  void WriteCampaignFile(std::string const & campaignFile);

  void UpdateFeaturesCache(df::CustomSymbols const & symbols);

  void FillSupportedTypes();

  GetMwmsByRectFn m_getMwmsByRectFn;
  GetMwmIdByName m_getMwmIdByNameFn;

  ref_ptr<df::DrapeEngine> m_drapeEngine;

  std::map<std::string, bool> m_campaigns;
  struct CampaignInfo
  {
    Timestamp m_created;
    std::vector<uint8_t> m_data;
  };
  std::map<std::string, CampaignInfo> m_info;

  df::CustomSymbols m_symbolsCache;
  std::mutex m_symbolsCacheMutex;

  bool m_isRunning = false;
  std::condition_variable m_condition;
  std::set<Request> m_requestedCampaigns;
  std::mutex m_mutex;
  threads::SimpleThread m_thread;

  local_ads::Statistics m_statistics;

  std::set<FeatureID> m_featuresCache;
  mutable std::mutex m_featuresCacheMutex;

  ftypes::HashSetMatcher<uint32_t> m_supportedTypes;
};
