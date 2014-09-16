#include "routing_session.hpp"
#include "../indexer/mercator.hpp"

namespace routing
{

RoutingSession::RoutingSession(IRouter * router)
  : m_router(router)
  , m_route(string())
  , m_state(RouteNotReady)
{
}

void RoutingSession::BuildRoute(m2::PointD const & startPoint, m2::PointD const & endPoint,
                                IRouter::ReadyCallback const & callback)
{
  m_router->SetFinalPoint(endPoint);
  RebuildRoute(startPoint, callback);
}

void RoutingSession::RebuildRoute(m2::PointD const & startPoint, IRouter::ReadyCallback const & callback)
{
  m_state = RouteNotReady;

  m2::RectD const errorRect = MercatorBounds::RectByCenterXYAndSizeInMeters(startPoint, 20);
  m_tolerance = (errorRect.SizeX() + errorRect.SizeY()) / 2.0;

  m_router->CalculateRoute(startPoint,  [this, callback](Route const & route)
                                        {
                                          m_state = RouteNotStarted;
                                          m_route = route;
                                          callback(route);
                                        });
}

RoutingSession::State RoutingSession::OnLocationPositionChanged(m2::PointD const & position,
                                                                double errorRadius)
{
  switch (m_state)
  {
  case OnRoute:
    if (IsOnDestPoint(position, errorRadius))
      m_state = RouteFinished;
    else if (!IsOnRoute(position, errorRadius))
      m_state = RouteLeft;
    break;
  case RouteNotStarted:
    m_state = IsOnRoute(position, errorRadius) ? OnRoute : RouteNotReady;
    break;
  default:
    break;
  }

  return m_state;
}

bool RoutingSession::IsOnRoute(m2::PointD const & position, double errorRadius) const
{
  if (errorRadius > m_tolerance || m_route.GetPoly().GetShortestSquareDistance(position) < errorRadius)
    return true;

  return false;
}

bool RoutingSession::IsOnDestPoint(m2::PointD const & position, double errorRadius) const
{
  if (errorRadius > m_tolerance)
    return false;

  if (m_route.GetPoly().Back().SquareLength(position) < errorRadius * errorRadius)
    return true;
  return false;
}

}
