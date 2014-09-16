#pragma once

#include "../routing/router.hpp"
#include "../routing/route.hpp"

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
    RouteNotReady,
    RouteNotStarted,
    OnRoute,
    RouteLeft,
    RouteFinished
  };

  /// startPoint and destPoint in mercator
  RoutingSession(IRouter * router);

  void BuildRoute(m2::PointD const & startPoint, m2::PointD const & endPoint,
                  IRouter::ReadyCallback const & callback);
  void RebuildRoute(m2::PointD const & startPoint, IRouter::ReadyCallback const & callback);

  State OnLocationPositionChanged(m2::PointD const & position, double errorRadius);

private:
  // errorRadius is error in determining the position in the Mercator
  bool IsOnRoute(m2::PointD const & position, double errorRadius) const;
  bool IsOnDestPoint(m2::PointD const & position, double errorRadius) const;

private:
  unique_ptr<IRouter> m_router;
  Route m_route;
  State m_state;

  double m_tolerance;
};

}
