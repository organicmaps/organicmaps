#pragma once

#include "traffic/traffic_info.hpp"

#include "drape_frontend/traffic_generator.hpp"

#include "drape/pointers.hpp"

#include "geometry/point2d.hpp"
#include "geometry/polyline2d.hpp"
#include "geometry/screenbase.hpp"

#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"

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

  using GetMwmsByRectFn = function<vector<MwmSet::MwmId>(m2::RectD const &)>;

  TrafficManager(GetMwmsByRectFn const & getMwmsByRectFn, size_t maxCacheSizeBytes);
  ~TrafficManager();

  void SetEnabled(bool enabled);

  void UpdateViewport(ScreenBase const & screen);
  void UpdateMyPosition(MyPosition const & myPosition);

  void OnRecover();

  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);

private:
  void ThreadRoutine();
  bool WaitForRequest(vector<MwmSet::MwmId> & mwms);
  void RequestTrafficData();
  void RequestTrafficData(MwmSet::MwmId const & mwmId);
  void OnTrafficDataResponse(traffic::TrafficInfo const & info);
  void CheckCacheSize();

  bool m_isEnabled;

  GetMwmsByRectFn m_getMwmsByRectFn;

  ref_ptr<df::DrapeEngine> m_drapeEngine;

  MyPosition m_currentPosition;
  ScreenBase m_currentModelView;

  struct MwmTrafficInfo
  {
    MwmTrafficInfo() = default;

    MwmTrafficInfo(time_point<steady_clock> const & requestTime) : m_lastRequestTime(requestTime) {}
    bool m_isLoaded = false;
    time_point<steady_clock> m_lastSeenTime;
    time_point<steady_clock> m_lastRequestTime;
    size_t m_dataSize = 0;
  };

  size_t m_maxCacheSizeBytes;
  size_t m_currentCacheSizeBytes;

  map<MwmSet::MwmId, MwmTrafficInfo> m_mwmInfos;

  bool m_isRunning;
  condition_variable m_condition;

  vector<MwmSet::MwmId> m_activeMwms;

  vector<MwmSet::MwmId> m_requestedMwms;
  mutex m_requestedMwmsLock;
  thread m_thread;
};
