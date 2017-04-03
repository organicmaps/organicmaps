#pragma once

#include "drape_frontend/custom_symbol.hpp"

#include "drape/pointers.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"

#include "base/thread.hpp"

#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace df
{
class DrapeEngine;
}  // namespace df

class LocalAdsManager final
{
public:
  using GetMwmsByRectFn = function<std::vector<MwmSet::MwmId>(m2::RectD const &)>;
  using GetMwmIdByName = function<MwmSet::MwmId(std::string const &)>;

  LocalAdsManager(GetMwmsByRectFn const & getMwmsByRectFn, GetMwmIdByName const & getMwmIdByName);
  LocalAdsManager(LocalAdsManager && /* localAdsManager */) = default;
  ~LocalAdsManager();

  void Teardown();
  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);
  void UpdateViewport(ScreenBase const & screen);

  void OnDownloadCountry(std::string const & countryName);
  void OnDeleteCountry(std::string const & countryName);

private:
  enum class RequestType
  {
    Download,
    Delete
  };
  using Request = std::pair<MwmSet::MwmId, RequestType>;

  void ThreadRoutine();
  bool WaitForRequest(std::vector<Request> & campaignMwms);

  std::string MakeRemoteURL(MwmSet::MwmId const & mwmId) const;
  std::vector<uint8_t> DownloadCampaign(MwmSet::MwmId const & mwmId) const;
  df::CustomSymbols DeserializeCampaign(std::vector<uint8_t> && rawData,
                                        std::chrono::steady_clock::time_point timestamp);
  void SendSymbolsToRendering(df::CustomSymbols && symbols);
  void DeleteSymbolsFromRendering(MwmSet::MwmId const & mwmId);

  void ReadExpirationFile(std::string const & expirationFile);
  void ReadCampaignFile(std::string const & campaignFile);

  GetMwmsByRectFn m_getMwmsByRectFn;
  GetMwmIdByName m_getMwmIdByNameFn;

  ref_ptr<df::DrapeEngine> m_drapeEngine;

  std::map<std::string, bool> m_campaigns;
  std::map<std::string, std::chrono::steady_clock::time_point> m_expiration;

  df::CustomSymbols m_symbolsCache;
  std::mutex m_symbolsCacheMutex;

  bool m_isRunning = true;
  std::condition_variable m_condition;
  std::vector<Request> m_requestedCampaigns;
  std::mutex m_mutex;
  threads::SimpleThread m_thread;
};
