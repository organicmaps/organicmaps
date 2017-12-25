#pragma once

#include "routing/async_router.hpp"
#include "routing/route.hpp"
#include "routing/router.hpp"
#include "routing/turns.hpp"
#include "routing/turns_notification_manager.hpp"

#include "indexer/index.hpp"

#include "traffic/speed_groups.hpp"
#include "traffic/traffic_cache.hpp"
#include "traffic/traffic_info.hpp"

#include "platform/location.hpp"
#include "platform/measurement_utils.hpp"

#include "geometry/point2d.hpp"
#include "geometry/polyline2d.hpp"

#include "base/mutex.hpp"

#include "std/atomic.hpp"
#include "std/limits.hpp"
#include "std/map.hpp"
#include "std/shared_ptr.hpp"
#include "std/unique_ptr.hpp"

namespace location
{
class RouteMatchingInfo;
}

namespace routing
{
struct SpeedCameraRestriction
{
  size_t m_index;  // Index of a polyline point where camera is located.
  uint8_t m_maxSpeedKmH;  // Maximum speed allowed by the camera.

  SpeedCameraRestriction(size_t index, uint8_t maxSpeed) : m_index(index), m_maxSpeedKmH(maxSpeed) {}
  SpeedCameraRestriction() : m_index(0), m_maxSpeedKmH(numeric_limits<uint8_t>::max()) {}
};

class RoutingSession : public traffic::TrafficObserver, public traffic::TrafficCache
{
  friend void UnitTest_TestFollowRoutePercentTest();

public:
  enum State
  {
    RoutingNotActive,
    RouteBuilding,     // we requested a route and wait when it will be builded
    RouteNotReady,     // routing was not build
    RouteNotStarted,   // route is builded but the user isn't on it
    OnRoute,           // user follows the route
    RouteNeedRebuild,  // user left the route
    RouteFinished,     // destination point is reached but the session isn't closed
    RouteNoFollowing,  // route is built but following mode has been disabled
    RouteRebuilding,   // we requested a route rebuild and wait when it will be rebuilded
  };

  /*
   * RoutingNotActive -> RouteBuilding    // start route building
   * RouteBuilding -> RouteNotReady       // waiting for route in case of building a new route
   * RouteBuilding -> RouteNotStarted     // route is builded in case of building a new route
   * RouteRebuilding -> RouteNotReady     // waiting for route in case of rebuilding
   * RouteRebuilding -> RouteNotStarted   // route is builded in case of rebuilding
   * RouteNotStarted -> OnRoute           // user started following the route
   * RouteNotStarted -> RouteNeedRebuild  // user doesn't like the route.
   * OnRoute -> RouteNeedRebuild          // user moves away from route - need to rebuild
   * OnRoute -> RouteNoFollowing          // following mode was disabled. Router doesn't track position.
   * OnRoute -> RouteFinished             // user reached the end of route
   * RouteNeedRebuild -> RouteNotReady    // start rebuild route
   * RouteFinished -> RouteNotReady       // start new route
   */

  typedef function<void(map<string, string> const &)> TRoutingStatisticsCallback;

  typedef function<void(Route const &, IRouter::ResultCode)> TReadyCallback;
  typedef function<void(float)> TProgressCallback;
  typedef function<void(size_t passedCheckpointIdx)> CheckpointCallback;

  RoutingSession();

  void Init(TRoutingStatisticsCallback const & routingStatisticsFn,
            RouterDelegate::TPointCheckCallback const & pointCheckCallback);

  void SetRouter(unique_ptr<IRouter> && router, unique_ptr<OnlineAbsentCountriesFetcher> && fetcher);

  /// @param[in] checkpoints in mercator
  /// @param[in] timeoutSec timeout in seconds, if zero then there is no timeout
  void BuildRoute(Checkpoints const & checkpoints,
                  uint32_t timeoutSec);
  void RebuildRoute(m2::PointD const & startPoint, TReadyCallback const & readyCallback,
                    uint32_t timeoutSec, State routeRebuildingState, bool adjustToPrevRoute);

  m2::PointD GetStartPoint() const;
  m2::PointD GetEndPoint() const;
  bool IsActive() const { return (m_state != RoutingNotActive); }
  bool IsNavigable() const { return (m_state == RouteNotStarted || m_state == OnRoute || m_state == RouteFinished); }
  bool IsBuilt() const { return (IsNavigable() || m_state == RouteNeedRebuild); }
  /// \returns true if a new route is in process of building rebuilding or
  /// if a route is being rebuilt in case the user left the route, and false otherwise.
  bool IsBuilding() const { return (m_state == RouteBuilding || m_state == RouteRebuilding); }
  bool IsBuildingOnly() const { return m_state == RouteBuilding; }
  bool IsRebuildingOnly() const { return m_state == RouteRebuilding; }
  bool IsNotReady() const { return m_state == RouteNotReady; }
  bool IsFinished() const { return m_state == RouteFinished; }
  bool IsNoFollowing() const { return m_state == RouteNoFollowing; }
  bool IsOnRoute() const { return (m_state == OnRoute); }
  bool IsFollowing() const { return m_isFollowing; }
  void Reset();

  inline void SetState(State state) { m_state = state; }

  shared_ptr<Route> const GetRoute() const;
  /// \returns true if altitude information along |m_route| is available and
  /// false otherwise.
  bool HasRouteAltitude() const;

  /// \brief copies distance from route beginning to ends of route segments in meters and
  /// route altitude information to |routeSegDistanceM| and |routeAltitudes|.
  /// \returns true if there is valid route information. If the route is not valid returns false.
  bool GetRouteAltitudesAndDistancesM(vector<double> & routeSegDistanceM,
                                      feature::TAltitudes & routeAltitudesM) const;

  State OnLocationPositionChanged(location::GpsInfo const & info, Index const & index);
  void GetRouteFollowingInfo(location::FollowingInfo & info) const;

  void MatchLocationToRoute(location::GpsInfo & location,
                            location::RouteMatchingInfo & routeMatchingInfo) const;
  // Get traffic speed for the current route position.
  // Returns SpeedGroup::Unknown if any trouble happens: position doesn't match with route or something else.
  traffic::SpeedGroup MatchTraffic(location::RouteMatchingInfo const & routeMatchingInfo) const;

  void SetUserCurrentPosition(m2::PointD const & position);

  void ActivateAdditionalFeatures() {}

  /// Disable following mode on GPS updates. Following mode is disabled only for the current route.
  /// If a route is rebuilt you must call DisableFollowMode again.
  /// Returns true if following was disabled, false if a route is not ready for the following yet.
  bool DisableFollowMode();

  /// Now indicates only that user pushed start button. Route follows by default.
  /// Returns true if following was enabled, false if a route is not ready for the following yet.
  bool EnableFollowMode();

  void SetRoutingSettings(RoutingSettings const & routingSettings);
  void SetReadyCallbacks(TReadyCallback const & buildReadyCallback,
                         TReadyCallback const & rebuildReadyCallback);
  void SetProgressCallback(TProgressCallback const & progressCallback);
  void SetCheckpointCallback(CheckpointCallback const & checkpointCallback);

  // Sound notifications for turn instructions.
  void EnableTurnNotifications(bool enable);
  bool AreTurnNotificationsEnabled() const;
  void SetTurnNotificationsUnits(measurement_utils::Units const units);
  void SetTurnNotificationsLocale(string const & locale);
  string GetTurnNotificationsLocale() const;
  void GenerateTurnNotifications(vector<string> & turnNotifications);

  void EmitCloseRoutingEvent() const;

  // RoutingObserver overrides:
  void OnTrafficInfoClear() override;
  void OnTrafficInfoAdded(traffic::TrafficInfo && info) override;
  void OnTrafficInfoRemoved(MwmSet::MwmId const & mwmId) override;

  // TrafficCache overrides:
  shared_ptr<traffic::TrafficInfo::Coloring> GetTrafficInfo(MwmSet::MwmId const & mwmId) const override;
  void CopyTraffic(std::map<MwmSet::MwmId, std::shared_ptr<traffic::TrafficInfo::Coloring>> & trafficColoring) const override;

private:
  struct DoReadyCallback
  {
    RoutingSession & m_rs;
    TReadyCallback m_callback;
    threads::Mutex & m_routeSessionMutexInner;

    DoReadyCallback(RoutingSession & rs, TReadyCallback const & cb,
                    threads::Mutex & routeSessionMutex)
        : m_rs(rs), m_callback(cb), m_routeSessionMutexInner(routeSessionMutex)
    {
    }

    void operator()(Route & route, IRouter::ResultCode e);
  };

  void AssignRoute(Route & route, IRouter::ResultCode e);

  /// Returns a nearest speed camera record on your way and distance to it.
  /// Returns kInvalidSpeedCameraDistance if there is no cameras on your way.
  double GetDistanceToCurrentCamM(SpeedCameraRestriction & camera, Index const & index);

  /// RemoveRoute removes m_route and resets route attributes (m_state, m_lastDistance, m_moveAwayCounter).
  void RemoveRoute();
  void RebuildRouteOnTrafficUpdate();

  // Must be called with locked m_routingSessionMutex
  void ResetImpl();

  double GetCompletionPercent() const;
  void PassCheckpoints();

private:
  unique_ptr<AsyncRouter> m_router;
  shared_ptr<Route> m_route;
  atomic<State> m_state;
  atomic<bool> m_isFollowing;
  Checkpoints m_checkpoints;
  size_t m_lastWarnedSpeedCameraIndex;
  SpeedCameraRestriction m_lastFoundCamera;
  // Index of a last point on a route checked for a speed camera.
  size_t m_lastCheckedSpeedCameraIndex;

  // TODO (ldragunov) Rewrite UI interop to message queue and avoid mutable.
  /// This field is mutable because it's modified in a constant getter. Note that the notification
  /// about camera will be sent at most once.
  mutable bool m_speedWarningSignal;

  mutable threads::Mutex m_routingSessionMutex;

  /// Current position metrics to check for RouteNeedRebuild state.
  double m_lastDistance;
  int m_moveAwayCounter;
  m2::PointD m_lastGoodPosition;
  // |m_currentDirection| is a vector from the position before last good position to last good position.
  m2::PointD m_currentDirection;
  m2::PointD m_userCurrentPosition;
  m2::PointD m_userFormerPosition;
  bool m_userCurrentPositionValid = false;
  bool m_userFormerPositionValid = false;

  // Sound turn notification parameters.
  turns::sound::NotificationManager m_turnNotificationsMgr;

  RoutingSettings m_routingSettings;

  TReadyCallback m_buildReadyCallback;
  TReadyCallback m_rebuildReadyCallback;
  TProgressCallback m_progressCallback;
  CheckpointCallback m_checkpointCallback;

  // Statistics parameters
  // Passed distance on route including reroutes
  double m_passedDistanceOnRouteMeters;
  // Rerouting count
  int m_routingRebuildCount;
  mutable double m_lastCompletionPercent;
};

void FormatDistance(double dist, string & value, string & suffix);

string DebugPrint(RoutingSession::State state);
}  // namespace routing
