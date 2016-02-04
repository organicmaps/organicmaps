#pragma once

#include "routing/async_router.hpp"
#include "routing/route.hpp"
#include "routing/router.hpp"
#include "routing/turns.hpp"
#include "routing/turns_notification_manager.hpp"

#include "platform/location.hpp"

#include "geometry/point2d.hpp"
#include "geometry/polyline2d.hpp"

#include "base/mutex.hpp"

#include "std/atomic.hpp"
#include "std/limits.hpp"
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

class RoutingSession
{
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
    RouteNoFollowing   // route is built but following mode has been disabled
  };

  /*
   * RoutingNotActive -> RouteBuilding    // start route building
   * RouteBuilding -> RouteNotReady       // waiting for route
   * RouteBuilding -> RouteNotStarted     // route is builded
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

  RoutingSession();

  void Init(TRoutingStatisticsCallback const & routingStatisticsFn,
            RouterDelegate::TPointCheckCallback const & pointCheckCallback);

  void SetRouter(unique_ptr<IRouter> && router, unique_ptr<OnlineAbsentCountriesFetcher> && fetcher);

  /// @param[in] startPoint and endPoint in mercator
  /// @param[in] timeoutSec timeout in seconds, if zero then there is no timeout
  void BuildRoute(m2::PointD const & startPoint, m2::PointD const & endPoint,
                  TReadyCallback const & readyCallback,
                  TProgressCallback const & progressCallback, uint32_t timeoutSec);
  void RebuildRoute(m2::PointD const & startPoint, TReadyCallback const & readyCallback,
                    TProgressCallback const & progressCallback, uint32_t timeoutSec);

  m2::PointD GetEndPoint() const { return m_endPoint; }
  bool IsActive() const { return (m_state != RoutingNotActive); }
  bool IsNavigable() const { return (m_state == RouteNotStarted || m_state == OnRoute || m_state == RouteFinished); }
  bool IsBuilt() const { return (IsNavigable() || m_state == RouteNeedRebuild || m_state == RouteFinished); }
  bool IsBuilding() const { return (m_state == RouteBuilding); }
  bool IsOnRoute() const { return (m_state == OnRoute); }
  bool IsFollowing() const { return m_isFollowing; }
  void Reset();

  Route const & GetRoute() const { return m_route; }

  State OnLocationPositionChanged(location::GpsInfo const & info, Index const & index);
  void GetRouteFollowingInfo(location::FollowingInfo & info) const;

  void MatchLocationToRoute(location::GpsInfo & location,
                            location::RouteMatchingInfo & routeMatchingInfo) const;

  void SetUserCurrentPosition(m2::PointD const & position);
  m2::PointD const & GetUserCurrentPosition() const;

  void ActivateAdditionalFeatures() {}

  /// Disable following mode on GPS updates. Following mode is disabled only for the current route.
  /// If a route is rebuilt you must call DisableFollowMode again.
  /// Returns true if following was disabled, false if a route is not ready for the following yet.
  bool DisableFollowMode();

  /// Now indicates only that user pushed start button. Route follows by default.
  /// Returns true if following was enabled, false if a route is not ready for the following yet.
  bool EnableFollowMode();

  void SetRoutingSettings(RoutingSettings const & routingSettings);

  // Sound notifications for turn instructions.
  void EnableTurnNotifications(bool enable);
  bool AreTurnNotificationsEnabled() const;
  void SetTurnNotificationsUnits(Settings::Units const units);
  void SetTurnNotificationsLocale(string const & locale);
  string GetTurnNotificationsLocale() const;
  void GenerateTurnNotifications(vector<string> & turnNotifications);
  double GetCompletionPercent() const;

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
  void RemoveRouteImpl();

private:
  unique_ptr<AsyncRouter> m_router;
  Route m_route;
  atomic<State> m_state;
  atomic<bool> m_isFollowing;
  m2::PointD m_endPoint;
  size_t m_lastWarnedSpeedCameraIndex;
  SpeedCameraRestriction m_lastFoundCamera;
  // Index of a last point on a route checked for a speed camera.
  size_t m_lastCheckedSpeedCameraIndex;

  // TODO (ldragunov) Rewrite UI interop to message queue and avoid mutable.
  /// This field is mutable because it's modified in a constant getter. Note that the notification
  /// about camera will be sent at most once.
  mutable bool m_speedWarningSignal;

  mutable threads::Mutex m_routeSessionMutex;

  /// Current position metrics to check for RouteNeedRebuild state.
  double m_lastDistance;
  int m_moveAwayCounter;
  m2::PointD m_lastGoodPosition;
  m2::PointD m_userCurrentPosition;

  // Sound turn notification parameters.
  turns::sound::NotificationManager m_turnNotificationsMgr;

  RoutingSettings m_routingSettings;

  // Statistics parameters
  // Passed distance on route including reroutes
  double m_passedDistanceOnRouteMeters;
  // Rerouting count
  int m_routingRebuildCount;
  mutable double m_lastCompletionPercent;
};
}  // namespace routing
