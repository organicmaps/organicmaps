#pragma once

#include "traffic/traffic_info.hpp"

#include "drape_frontend/traffic_generator.hpp"

#include "drape/pointers.hpp"

#include "geometry/point2d.hpp"
#include "geometry/polyline2d.hpp"
#include "geometry/screenbase.hpp"

#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"

#include "base/thread.hpp"

#include "std/atomic.hpp"
#include "std/chrono.hpp"
#include "std/map.hpp"
#include "std/mutex.hpp"
#include "std/set.hpp"
#include "std/vector.hpp"

namespace df
{
class DrapeEngine;
}  // namespace df

class TrafficManager final
{
public:
  enum class TrafficState
  {
    Disabled,
    Enabled,
    WaitingData,
    Outdated,
    NoData,
    NetworkError,
    ExpiredData,
    ExpiredApp
  };

  struct MyPosition
  {
    m2::PointD m_position = m2::PointD(0.0, 0.0);
    bool m_knownPosition = false;

    MyPosition() = default;
    MyPosition(m2::PointD const & position)
      : m_position(position),
        m_knownPosition(true)
    {}
  };

  using TrafficStateChangedFn = function<void(TrafficState)>;
  using GetMwmsByRectFn = function<vector<MwmSet::MwmId>(m2::RectD const &)>;

  TrafficManager(GetMwmsByRectFn const & getMwmsByRectFn, size_t maxCacheSizeBytes,
                 traffic::TrafficObserver & observer);
  ~TrafficManager();

  void Teardown();

  void SetStateListener(TrafficStateChangedFn const & onStateChangedFn);
  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);
  void SetCurrentDataVersion(int64_t dataVersion);

  void SetEnabled(bool enabled);

  void UpdateViewport(ScreenBase const & screen);
  void UpdateMyPosition(MyPosition const & myPosition);

  void Invalidate();

  void OnDestroyGLContext();
  void OnRecoverGLContext();
  void OnMwmDelete(MwmSet::MwmId const & mwmId);

private:
  void ThreadRoutine();
  bool WaitForRequest(vector<MwmSet::MwmId> & mwms);

  void OnTrafficDataResponse(traffic::TrafficInfo && info);
  void OnTrafficRequestFailed(traffic::TrafficInfo && info);

private:
  // This is a group of methods that haven't their own synchronization inside.
  void RequestTrafficData();
  void RequestTrafficData(MwmSet::MwmId const & mwmId, bool force);

  void Clear();
  void ClearCache(MwmSet::MwmId const & mwmId);
  void CheckCacheSize();

  void UpdateState();
  void ChangeState(TrafficState newState);

  bool IsInvalidState() const;
  bool IsEnabled() const;

  GetMwmsByRectFn m_getMwmsByRectFn;
  traffic::TrafficObserver & m_observer;

  ref_ptr<df::DrapeEngine> m_drapeEngine;
  atomic<int64_t> m_currentDataVersion;

  // These fields have a flag of their initialization.
  pair<MyPosition, bool> m_currentPosition = {MyPosition(), false};
  pair<ScreenBase, bool> m_currentModelView = {ScreenBase(), false};

  atomic<TrafficState> m_state;
  TrafficStateChangedFn m_onStateChangedFn;

  struct CacheEntry
  {
    CacheEntry();
    CacheEntry(time_point<steady_clock> const & requestTime);

    bool m_isLoaded;
    size_t m_dataSize;

    time_point<steady_clock> m_lastSeenTime;
    time_point<steady_clock> m_lastRequestTime;
    time_point<steady_clock> m_lastResponseTime;

    int m_retriesCount;
    bool m_isWaitingForResponse;

    traffic::TrafficInfo::Availability m_lastAvailability;
  };

  size_t m_maxCacheSizeBytes;
  size_t m_currentCacheSizeBytes = 0;

  map<MwmSet::MwmId, CacheEntry> m_mwmCache;

  bool m_isRunning;
  condition_variable m_condition;

  vector<MwmSet::MwmId> m_lastMwmsByRect;
  set<MwmSet::MwmId> m_activeMwms;

  vector<MwmSet::MwmId> m_requestedMwms;
  mutex m_mutex;
  threads::SimpleThread m_thread;
};

extern string DebugPrint(TrafficManager::TrafficState state);
