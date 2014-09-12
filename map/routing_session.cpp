#include "routing_session.hpp"

namespace routing
{

RoutingSession::RoutingSession(IRouter * router)
  : m_router(router)
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
  m_router->CalculateRoute(startPoint,  [this, callback](Route const & route)
                                        {
                                          m_state = RouteNotStarted;
                                          m_routeGeometry = route.GetPoly();
                                          callback(route);
                                        });
}

RoutingSession::State RoutingSession::OnLocationPositionChanged(m2::PointD const & position,
                                                                double errorRadius)
{
  if (m_state == RouteNotReady || m_state == RouteFinished || m_state == RouteLeft)
    return m_state;

  if (m_state == OnRoute && IsOnDestPoint(position, errorRadius))
  {
    m_state = RouteFinished;
    return m_state;
  }

  bool isOnRoute = IsOnRoute(position, errorRadius);
  if (isOnRoute)
    m_state = OnRoute;
  else if (m_state != RouteNotStarted)
    m_state = RouteLeft;

  return m_state;
}

bool RoutingSession::IsOnRoute(m2::PointD const & position, double errorRadius) const
{
  if (errorRadius > MaxValidError || m_routeGeometry.GetShortestSquareDistance(position) < errorRadius)
    return true;

  return false;
}

bool RoutingSession::IsOnDestPoint(m2::PointD const & position, double errorRadius) const
{
  if (errorRadius > MaxValidError)
    return false;

  m2::PointD lastPoint = *reverse_iterator<m2::PolylineD::TIter>(m_routeGeometry.End());
  if (lastPoint.SquareLength(position) < errorRadius * errorRadius)
    return true;
  return false;
}

}
