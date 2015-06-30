#pragma once

#include "routing/async_router.hpp"
#include "routing/route.hpp"
#include "routing/router.hpp"
#include "routing/turns.hpp"
#include "routing/turns_sound.hpp"

#include "platform/location.hpp"

#include "geometry/point2d.hpp"
#include "geometry/polyline2d.hpp"

#include "std/unique_ptr.hpp"

#include "base/mutex.hpp"

namespace location
{
class RouteMatchingInfo;
}

namespace routing
{
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
    RouteFinished      // destination point is reached but the session isn't closed
  };

  /*
   * RoutingNotActive -> RouteBuilding    // start route building
   * RouteBuilding -> RouteNotReady       // waiting for route
   * RouteBuilding -> RouteNotStarted     // route is builded
   * RouteNotStarted -> OnRoute           // user started following the route
   * RouteNotStarted -> RouteNeedRebuild  // user doesn't like the route.
   * OnRoute -> RouteNeedRebuild          // user moves away from route - need to rebuild
   * OnRoute -> RouteFinished             // user reached the end of route
   * RouteNeedRebuild -> RouteNotReady    // start rebuild route
   * RouteFinished -> RouteNotReady       // start new route
   */

  typedef function<void(map<string, string> const &)> TRoutingStatisticsCallback;

  typedef function<void(Route const &, IRouter::ResultCode)> TReadyCallbackFn;

  RoutingSession();

  void SetRouter(unique_ptr<IRouter> && router, unique_ptr<OnlineAbsentCountriesFetcher> && fetcher,
                 TRoutingStatisticsCallback const & routingStatisticsFn);

  /// @param[in] startPoint and endPoint in mercator
  void BuildRoute(m2::PointD const & startPoint, m2::PointD const & endPoint,
                  TReadyCallbackFn const & callback);
  void RebuildRoute(m2::PointD const & startPoint, TReadyCallbackFn const & callback);

  m2::PointD GetEndPoint() const { return m_endPoint; }
  bool IsActive() const { return (m_state != RoutingNotActive); }
  bool IsNavigable() const { return (m_state == RouteNotStarted || m_state == OnRoute); }
  bool IsBuilt() const { return (IsNavigable() || m_state == RouteNeedRebuild || m_state == RouteFinished); }
  bool IsBuilding() const { return (m_state == RouteBuilding); }
  void Reset();

  State OnLocationPositionChanged(m2::PointD const & position, location::GpsInfo const & info);
  void GetRouteFollowingInfo(location::FollowingInfo & info) const;

  void MatchLocationToRoute(location::GpsInfo & location,
                            location::RouteMatchingInfo & routeMatchingInfo) const;

  // TODO (Dragunov) Make activation of the pedestrian routing
  void ActivateAdditionalFeatures() {}

  // Sound notifications for turn instructions.
  void AssignTurnSoundNotificationSettings(turns::sound::Settings const & settings);
  void EnableTurnNotification(bool enable);
  bool IsTurnNotificationEnabled() const;

private:
  struct DoReadyCallback
  {
    RoutingSession & m_rs;
    TReadyCallbackFn m_callback;
    threads::Mutex & m_routeSessionMutexInner;

    DoReadyCallback(RoutingSession & rs, TReadyCallbackFn const & cb,
                    threads::Mutex & routeSessionMutex)
        : m_rs(rs), m_callback(cb), m_routeSessionMutexInner(routeSessionMutex)
    {
    }

    void operator()(Route & route, IRouter::ResultCode e);
  };
  void AssignRoute(Route & route);
  /// RemoveRoute removes m_route and resets route attributes (m_state, m_lastDistance, m_moveAwayCounter).
  void RemoveRoute();
  void RemoveRouteImpl();

private:
  unique_ptr<AsyncRouter> m_router;
  Route m_route;
  State m_state;
  m2::PointD m_endPoint;

  mutable threads::Mutex m_routeSessionMutex;

  /// Current position metrics to check for RouteNeedRebuild state.
  double m_lastDistance;
  int m_moveAwayCounter;
  m2::PointD m_lastGoodPosition;

  // Sound turn notification parameters.
  turns::sound::TurnsSound m_turnsSound;
};
}  // namespace routing
