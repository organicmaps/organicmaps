#pragma once

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "transit/experimental/transit_data.hpp"
#include "transit/transit_display_info.hpp"

#include "indexer/data_source.hpp"
#include "indexer/feature_decl.hpp"

#include "geometry/screenbase.hpp"

#include "base/thread.hpp"
#include "base/thread_pool.hpp"

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

class DataSource;

using FeatureCallback = std::function<void(FeatureType &)>;
using TReadFeaturesFn = std::function<void(FeatureCallback const &, std::vector<FeatureID> const &)>;

class ReadTransitTask : public threads::IRoutine
{
public:
  ReadTransitTask(DataSource & dataSource, TReadFeaturesFn const & readFeaturesFn)
    : m_dataSource(dataSource)
    , m_readFeaturesFn(readFeaturesFn)
  {}

  void Init(uint64_t id, MwmSet::MwmId const & mwmId, std::unique_ptr<TransitDisplayInfo> transitInfo = nullptr);
  uint64_t GetId() const { return m_id; }
  bool GetSuccess() const { return m_success; }

  void Do() override;
  void Reset() override;

  std::unique_ptr<TransitDisplayInfo> && GetTransitInfo();

private:
  template <typename T, typename TID>
  void FillItemsByIdMap(std::vector<T> const & items, std::map<TID, T> & itemsById)
  {
    for (auto const & item : items)
    {
      if (!m_loadSubset)
      {
        itemsById.emplace(item.GetId(), item);
      }
      else
      {
        auto it = itemsById.find(item.GetId());
        if (it != itemsById.end())
          it->second = item;
      }
    }
  }

  void FillLinesAndRoutes(::transit::experimental::TransitData const & transitData);

  DataSource & m_dataSource;
  TReadFeaturesFn m_readFeaturesFn;

  uint64_t m_id = 0;
  MwmSet::MwmId m_mwmId;
  std::unique_ptr<TransitDisplayInfo> m_transitInfo;

  bool m_loadSubset = false;
  // Sets to true if Do() method was executed successfully.
  bool m_success = false;
};

class TransitReadManager
{
public:
  enum class TransitSchemeState
  {
    Disabled,
    Enabled,
    NoData,
  };

  using GetMwmsByRectFn = std::function<std::vector<MwmSet::MwmId>(m2::RectD const &)>;
  using TransitStateChangedFn = std::function<void(TransitSchemeState)>;

  TransitReadManager(DataSource & dataSource, TReadFeaturesFn const & readFeaturesFn,
                     GetMwmsByRectFn const & getMwmsByRectFn);
  ~TransitReadManager();

  void Start();
  void Stop();

  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);

  TransitSchemeState GetState() const;
  void SetStateListener(TransitStateChangedFn const & onStateChangedFn);

  bool GetTransitDisplayInfo(TransitDisplayInfos & transitDisplayInfos);

  void EnableTransitSchemeMode(bool enable);
  void BlockTransitSchemeMode(bool isBlocked);
  void UpdateViewport(ScreenBase const & screen);
  void OnMwmDeregistered(platform::LocalCountryFile const & countryFile);
  void Invalidate();

private:
  void OnTaskCompleted(threads::IRoutine * task);

  void ChangeState(TransitSchemeState newState);
  void ShrinkCacheToAllowableSize();
  void ClearCache(MwmSet::MwmId const & mwmId);

  std::unique_ptr<base::ThreadPool> m_threadsPool;

  std::mutex m_mutex;
  std::condition_variable m_event;

  uint64_t m_nextTasksGroupId = 0;
  std::map<uint64_t, size_t> m_tasksGroups;

  DataSource & m_dataSource;
  TReadFeaturesFn m_readFeaturesFn;

  df::DrapeEngineSafePtr m_drapeEngine;

  TransitSchemeState m_state = TransitSchemeState::Disabled;
  TransitStateChangedFn m_onStateChangedFn;

  struct CacheEntry
  {
    CacheEntry(std::chrono::time_point<std::chrono::steady_clock> const & activeTime) : m_lastActiveTime(activeTime) {}

    bool m_isLoaded = false;
    size_t m_dataSize = 0;
    std::chrono::time_point<std::chrono::steady_clock> m_lastActiveTime;
  };

  GetMwmsByRectFn m_getMwmsByRectFn;
  std::set<MwmSet::MwmId> m_lastActiveMwms;
  std::map<MwmSet::MwmId, CacheEntry> m_mwmCache;
  size_t m_cacheSize = 0;
  bool m_isSchemeMode = false;
  bool m_isSchemeModeBlocked = false;
  std::pair<ScreenBase, bool> m_currentModelView = {ScreenBase(), false /* initialized */};
};
