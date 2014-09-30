#include "routing_session.hpp"
#include "../indexer/mercator.hpp"

namespace routing
{

RoutingSession::RoutingSession()
  : m_router(nullptr)
  , m_route(string())
  , m_state(RoutingNotActive)
  , m_lastMinDist(0.0)
{
}

void RoutingSession::BuildRoute(m2::PointD const & startPoint, m2::PointD const & endPoint,
                                IRouter::ReadyCallback const & callback)
{
  ASSERT(m_router != nullptr, ());
  m_router->SetFinalPoint(endPoint);
  RebuildRoute(startPoint, callback);
}

void RoutingSession::RebuildRoute(m2::PointD const & startPoint, IRouter::ReadyCallback const & callback)
{
  ASSERT(m_router != nullptr, ());
  Reset();
  m_state = RouteNotReady;

  m2::RectD const errorRect = MercatorBounds::RectByCenterXYAndSizeInMeters(startPoint, 20);
  m_tolerance = (errorRect.SizeX() + errorRect.SizeY()) / 2.0;

  m_router->CalculateRoute(startPoint,  [this, callback](Route const & route, IRouter::ResultCode e)
  {
    if (route.GetPoly().GetSize() < 2)
      return;

    m_state = RouteNotStarted;
    m_route = route;
    callback(route, e);
  });
}

bool RoutingSession::IsActive() const
{
  return m_state != RoutingNotActive;
}

void RoutingSession::Reset()
{
  m_state = RoutingNotActive;
  m_lastMinDist = 0.0;
  m_route = Route(string());
}

RoutingSession::State RoutingSession::OnLocationPositionChanged(m2::PointD const & position,
                                                                double errorRadius)
{
  ASSERT(m_state != RoutingNotActive, ());
  ASSERT(m_router != nullptr, ());

  if (m_state == RouteNotReady || m_state == RouteLeft || m_state == RouteFinished)
    return m_state;

  if (IsOnDestPoint(position, errorRadius))
    m_state = RouteFinished;
  else
  {
    double currentDist = 0.0;
    if (IsOnRoute(position, errorRadius, currentDist))
    {
      m_state = OnRoute;
      m_moveAwayCounter = 0;
      m_lastMinDist = 0.0;
    }
    else
    {
      if (currentDist > m_lastMinDist)
      {
        ++m_moveAwayCounter;
        m_lastMinDist = currentDist;
      }
      else
      {
        m_moveAwayCounter = 0;
        m_lastMinDist = 0.0;
      }

      if (m_moveAwayCounter > 10)
        m_state = RouteLeft;
    }
  }

  return m_state;
}

bool RoutingSession::IsOnRoute(m2::PointD const & position, double errorRadius, double & minDist) const
{
  minDist = sqrt(m_route.GetPoly().GetShortestSquareDistance(position));
  if (errorRadius > m_tolerance || minDist < (errorRadius + m_tolerance))
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

void RoutingSession::SetRouter(IRouter * router)
{
  m_router.reset(router);
}

}
