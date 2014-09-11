#include "routing_session.hpp"

namespace routing
{

RoutingSession::RoutingSession(IRouter * router)
  : m_router(router)
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
  m_router->CalculateRoute(startPoint,  [this, callback](Route const & route)
                                        {
                                          m_routeGeometry = route.GetPoly();
                                          callback(route);
                                        });
}

RoutingSession::State RoutingSession::OnLocationPositionChanged(m2::PointD const & position,
                                                                double errorRadius)
{
  if (m_state != RoutFinished && m_state != RouteLeft)
  {
    if (IsOnDestPoint(position, errorRadius))
      m_state = RoutFinished;
    else
      m_state = IsOnRoute(position, errorRadius) ? OnRout : RouteLeft;
  }

  return m_state;
}

bool RoutingSession::IsOnRoute(m2::PointD const & position, double errorRadius) const
{
  return true;
}

bool RoutingSession::IsOnDestPoint(m2::PointD const & position, double errorRadius) const
{
  return false;
}

}
