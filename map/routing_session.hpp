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
    RouteNotReady,   // routing not active or we request route and wait when it will be builded
    RouteNotStarted, // route builded but user not on route
    OnRoute,         // user follows the route
    RouteLeft,       // user left from a route
    RouteFinished    // destination point is reached but session not closed
  };

  /*
   * RoutingNotActive -> RouteNotReady // wait route
   * RouteNotReady -> RouteNotStarted // rounte builded
   * RouteNotStarted -> OnRoute       // user start follow the route
   * RouteNotStarted -> RouteLeft     // user not like our route.
   *                                  // He go to the other side. Need to rebuild
   * OnRoute -> RouteLeft
   * OnRoute -> RouteFinished
   * RouteLeft -> RouteNotReady       // start rebuild route
   * RouteFinished -> RouteNotReady   // start new route
   */

  RoutingSession();
  void SetRouter(IRouter * router);

  typedef function<void (Route const &)> ReadyCallback;
  /// @param[in] startPoint and endPoint in mercator
  void BuildRoute(m2::PointD const & startPoint, m2::PointD const & endPoint,
                  ReadyCallback const & callback);

  void RebuildRoute(m2::PointD const & startPoint, ReadyCallback const & callback);
  bool IsActive() const;
  void Reset();

  State OnLocationPositionChanged(m2::PointD const & position, location::GpsInfo const & info);
  void GetRouteFollowingInfo(location::FollowingInfo & info) const;

private:
  void AssignRoute(Route & route);

private:
  unique_ptr<IRouter> m_router;
  Route m_route;
  State m_state;

  /// Current position metrics to check for RouteLeft state.
  double m_lastDistance;
  int m_moveAwayCounter;
};

}
