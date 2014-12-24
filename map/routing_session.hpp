#pragma once

#include "../routing/router.hpp"
#include "../routing/route.hpp"

#include "../platform/location.hpp"

#include "../geometry/point2d.hpp"
#include "../geometry/polyline2d.hpp"

#include "../std/unique_ptr.hpp"


namespace routing
{

class RoutingSession
{
public:
  enum State
  {
    RoutingNotActive,
    RouteBuilding,    // we requested a route and wait when it will be builded
    RouteNotReady,    // routing was not build
    RouteNotStarted,  // route is builded but the user isn't on it
    OnRoute,          // user follows the route
    RouteNeedRebuild, // user left the route
    RouteFinished     // destination point is reached but the session isn't closed
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

  RoutingSession();
  void SetRouter(IRouter * router);

  typedef function<void (Route const &, IRouter::ResultCode)> TReadyCallbackFn;

  /// @param[in] startPoint and endPoint in mercator
  void BuildRoute(m2::PointD const & startPoint, m2::PointD const & endPoint, TReadyCallbackFn const & callback);

  void RebuildRoute(m2::PointD const & startPoint, TReadyCallbackFn const & callback);
  bool IsActive() const;
  bool IsNavigable() const;
  void Reset();

  State OnLocationPositionChanged(m2::PointD const & position, location::GpsInfo const & info);
  void GetRouteFollowingInfo(location::FollowingInfo & info) const;

  void DeleteIndexFile(string const & fileName);
  void MatchLocationToRoute(location::GpsInfo & location) const;

private:
  struct DoReadyCallback
  {
    RoutingSession & m_rs;
    TReadyCallbackFn m_callback;

    DoReadyCallback(RoutingSession & rs, TReadyCallbackFn const & cb)
      : m_rs(rs), m_callback(cb)
    {
    }

    void operator() (Route & route, IRouter::ResultCode e);
  };

  void AssignRoute(Route & route);

private:
  unique_ptr<IRouter> m_router;
  Route m_route;
  State m_state;

  /// Current position metrics to check for RouteNeedRebuild state.
  double m_lastDistance;
  int m_moveAwayCounter;
  m2::PointD m_lastGoodPosition;
};

}
