#pragma once

#include "traffic/traffic_info.hpp"

#include "drape_frontend/traffic_generator.hpp"

#include "drape/pointers.hpp"

#include "geometry/point2d.hpp"
#include "geometry/polyline2d.hpp"
#include "geometry/screenbase.hpp"

#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"

#include "std/atomic.hpp"
#include "std/chrono.hpp"
#include "std/map.hpp"
#include "std/mutex.hpp"
#include "std/set.hpp"
#include "std/thread.hpp"
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

  TrafficManager(GetMwmsByRectFn const & getMwmsByRectFn, size_t maxCacheSizeBytes);
  ~TrafficManager();

  void SetStateListener(TrafficStateChangedFn const & onStateChangedFn);
  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);
  void SetCurrentDataVersion(int64_t dataVersion);

  void SetEnabled(bool enabled);

  void UpdateViewport(ScreenBase const & screen);
  void UpdateMyPosition(MyPosition const & myPosition);

  void OnDestroyGLContext();
  void OnRecoverGLContext();

private:
  void ThreadRoutine();
  bool WaitForRequest(vector<MwmSet::MwmId> & mwms);

  void RequestTrafficData();
  void RequestTrafficData(MwmSet::MwmId const & mwmId);

  void OnTrafficDataResponse(traffic::TrafficInfo const & info);
  void OnTrafficRequestFailed(traffic::TrafficInfo const & info);

  void Clear();
  void CheckCacheSize();

  void UpdateState();
  void ChangeState(TrafficState newState);
  void NotifyStateChanged();

  bool IsInvalidState() const;
  bool IsEnabled() const;

  GetMwmsByRectFn m_getMwmsByRectFn;

  ref_ptr<df::DrapeEngine> m_drapeEngine;
  int64_t m_currentDataVersion = 0;

  pair<MyPosition, bool> m_currentPosition = {MyPosition(), false};
  pair<ScreenBase, bool> m_currentModelView = {ScreenBase(), false};

  atomic<TrafficState> m_state;
  atomic<bool> m_notifyStateChanged;
  TrafficStateChangedFn m_onStateChangedFn;

  struct CacheEntry
  {
    CacheEntry() = default;

    CacheEntry(time_point<steady_clock> const & requestTime) : m_lastRequestTime(requestTime) {}

    bool m_isLoaded = false;
    size_t m_dataSize = 0;

    time_point<steady_clock> m_lastSeenTime;
    time_point<steady_clock> m_lastRequestTime;
    time_point<steady_clock> m_lastResponseTime;

    uint32_t m_retriesCount = 0;
    bool m_isWaitingForResponse = false;

    traffic::TrafficInfo::Availability m_lastAvailability =
        traffic::TrafficInfo::Availability::Unknown;
  };

  size_t m_maxCacheSizeBytes;
  size_t m_currentCacheSizeBytes = 0;

  map<MwmSet::MwmId, CacheEntry> m_mwmCache;

  bool m_isRunning;
  condition_variable m_condition;

  set<MwmSet::MwmId> m_activeMwms;

  vector<MwmSet::MwmId> m_requestedMwms;
  mutex m_requestedMwmsLock;
  thread m_thread;
};
