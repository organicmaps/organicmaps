#pragma once

#include "traffic/traffic_info.hpp"

#include "drape_frontend/drape_engine_safe_ptr.hpp"
#include "drape_frontend/traffic_generator.hpp"

#include "drape/pointers.hpp"

#include "geometry/point2d.hpp"
#include "geometry/polyline2d.hpp"
#include "geometry/screenbase.hpp"

#include "indexer/mwm_set.hpp"

#include "base/thread.hpp"

#include "std/algorithm.hpp"
#include "std/atomic.hpp"
#include "std/chrono.hpp"
#include "std/condition_variable.hpp"
#include "std/map.hpp"
#include "std/mutex.hpp"
#include "std/set.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

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
  TrafficManager(TrafficManager && /* trafficManager */) = default;
  ~TrafficManager();

  void Teardown();

  void SetStateListener(TrafficStateChangedFn const & onStateChangedFn);
  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);
  void SetCurrentDataVersion(int64_t dataVersion);

  void SetEnabled(bool enabled);
  bool IsEnabled() const;

  void UpdateViewport(ScreenBase const & screen);
  void UpdateMyPosition(MyPosition const & myPosition);

  void Invalidate();

  void OnDestroyGLContext();
  void OnRecoverGLContext();
  void OnMwmDeregistered(platform::LocalCountryFile const & countryFile);

  void OnEnterForeground();
  void OnEnterBackground();

  void SetSimplifiedColorScheme(bool simplified);
  bool HasSimplifiedColorScheme() const { return m_hasSimplifiedColorScheme; }

private:
  struct CacheEntry
  {
    CacheEntry();
    CacheEntry(time_point<steady_clock> const & requestTime);

    bool m_isLoaded;
    size_t m_dataSize;

    time_point<steady_clock> m_lastActiveTime;
    time_point<steady_clock> m_lastRequestTime;
    time_point<steady_clock> m_lastResponseTime;

    int m_retriesCount;
    bool m_isWaitingForResponse;

    traffic::TrafficInfo::Availability m_lastAvailability;
  };

  void ThreadRoutine();
  bool WaitForRequest(vector<MwmSet::MwmId> & mwms);

  void OnTrafficDataResponse(traffic::TrafficInfo && info);
  void OnTrafficRequestFailed(traffic::TrafficInfo && info);

  /// \brief Updates |activeMwms| and request traffic data.
  /// \param rect is a rectangle covering a new active mwm set.
  /// \note |lastMwmsByRect|/|activeMwms| may be either |m_lastDrapeMwmsByRect/|m_activeDrapeMwms|
  /// or |m_lastRoutingMwmsByRect|/|m_activeRoutingMwms|.
  /// \note |m_mutex| is locked inside the method. So the method should be called without |m_mutex|.
  void UpdateActiveMwms(m2::RectD const & rect, vector<MwmSet::MwmId> & lastMwmsByRect,
                        set<MwmSet::MwmId> & activeMwms);

  // This is a group of methods that haven't their own synchronization inside.
  void RequestTrafficData();
  void RequestTrafficData(MwmSet::MwmId const & mwmId, bool force);

  void Clear();
  void ClearCache(MwmSet::MwmId const & mwmId);
  void ShrinkCacheToAllowableSize();

  void UpdateState();
  void ChangeState(TrafficState newState);

  bool IsInvalidState() const;

  void UniteActiveMwms(set<MwmSet::MwmId> & activeMwms) const;

  void Pause();
  void Resume();

  template <class F>
  void ForEachActiveMwm(F && f) const
  {
    set<MwmSet::MwmId> activeMwms;
    UniteActiveMwms(activeMwms);
    for_each(activeMwms.begin(), activeMwms.end(), forward<F>(f));
  }

  GetMwmsByRectFn m_getMwmsByRectFn;
  traffic::TrafficObserver & m_observer;

  df::DrapeEngineSafePtr m_drapeEngine;
  atomic<int64_t> m_currentDataVersion;

  // These fields have a flag of their initialization.
  pair<MyPosition, bool> m_currentPosition = {MyPosition(), false};
  pair<ScreenBase, bool> m_currentModelView = {ScreenBase(), false};

  atomic<TrafficState> m_state;
  TrafficStateChangedFn m_onStateChangedFn;

  bool m_hasSimplifiedColorScheme = true;

  size_t m_maxCacheSizeBytes;
  size_t m_currentCacheSizeBytes = 0;

  map<MwmSet::MwmId, CacheEntry> m_mwmCache;

  bool m_isRunning;
  condition_variable m_condition;

  vector<MwmSet::MwmId> m_lastDrapeMwmsByRect;
  set<MwmSet::MwmId> m_activeDrapeMwms;
  vector<MwmSet::MwmId> m_lastRoutingMwmsByRect;
  set<MwmSet::MwmId> m_activeRoutingMwms;

  // The ETag or entity tag is part of HTTP, the protocol for the World Wide Web.
  // It is one of several mechanisms that HTTP provides for web cache validation,
  // which allows a client to make conditional requests.
  map<MwmSet::MwmId, string> m_trafficETags;

  atomic<bool> m_isPaused;

  vector<MwmSet::MwmId> m_requestedMwms;
  mutex m_mutex;
  threads::SimpleThread m_thread;
};

extern string DebugPrint(TrafficManager::TrafficState state);
