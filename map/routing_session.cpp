#include "routing_session.hpp"
#include "measurement_utils.hpp"

#include "../indexer/mercator.hpp"


using namespace location;

namespace routing
{

static int const ON_ROUTE_MISSED_COUNT = 10;


RoutingSession::RoutingSession()
  : m_router(nullptr)
  , m_route(string())
  , m_state(RoutingNotActive)
{
}

void RoutingSession::BuildRoute(m2::PointD const & startPoint, m2::PointD const & endPoint,
                                ReadyCallback const & callback)
{
  ASSERT(m_router != nullptr, ());
  m_router->SetFinalPoint(endPoint);
  RebuildRoute(startPoint, callback);
}

void RoutingSession::RebuildRoute(m2::PointD const & startPoint, ReadyCallback const & callback)
{
  ASSERT(m_router != nullptr, ());
  Reset();
  m_state = RouteNotReady;

  m_router->CalculateRoute(startPoint, [&] (Route & route, IRouter::ResultCode e)
  {
    if (e == IRouter::NoError)
    {
      AssignRoute(route);
      callback(m_route);
    }
    else
    {
      /// @todo Save error code here and return to the UI by demand.
    }
  });
}

bool RoutingSession::IsActive() const
{
  return (m_state != RoutingNotActive);
}

void RoutingSession::Reset()
{
  m_state = RoutingNotActive;
  m_lastDistance = 0.0;
  m_moveAwayCounter = 0;
  Route(string()).Swap(m_route);
}

RoutingSession::State RoutingSession::OnLocationPositionChanged(m2::PointD const & position,
                                                                GpsInfo const & info)
{
  ASSERT(m_state != RoutingNotActive, ());
  ASSERT(m_router != nullptr, ());

  if (m_state == RouteNotReady)
  {
    if (++m_moveAwayCounter > ON_ROUTE_MISSED_COUNT)
      return RouteNeedRebuild;
    else
      return RouteNotReady;
  }

  if (m_state == RouteNeedRebuild || m_state == RouteFinished)
    return m_state;

  ASSERT(m_route.IsValid(), ());

  if (m_route.MoveIterator(info))
  {
    m_moveAwayCounter = 0;
    m_lastDistance = 0.0;

    if (m_route.IsCurrentOnEnd())
      m_state = RouteFinished;
    else
      m_state = OnRoute;
  }
  else
  {
    // Distance from the last known projection on route
    // (check if we are moving far from the last known projection).
    double const dist = m_route.GetCurrentSqDistance(position);
    if (dist > m_lastDistance || my::AlmostEqual(dist, m_lastDistance, 1 << 16))
    {
      ++m_moveAwayCounter;
      m_lastDistance = dist;
    }
    else
    {
      m_moveAwayCounter = 0;
      m_lastDistance = 0.0;
    }

    if (m_moveAwayCounter > ON_ROUTE_MISSED_COUNT)
      m_state = RouteNeedRebuild;
  }

  return m_state;
}

void RoutingSession::GetRouteFollowingInfo(FollowingInfo & info) const
{
  if (m_route.IsValid())
  {
    /// @todo Make better formatting of distance and units.
    MeasurementUtils::FormatDistance(m_route.GetCurrentDistanceToEnd(), info.m_distToTarget);

    size_t const delim = info.m_distToTarget.find(' ');
    ASSERT(delim != string::npos, ());
    info.m_unitsSuffix = info.m_distToTarget.substr(delim + 1);
    info.m_distToTarget.erase(delim);
  }
}

void RoutingSession::AssignRoute(Route & route)
{
  ASSERT(route.IsValid(), ());
  m_state = RouteNotStarted;
  m_route.Swap(route);
}

void RoutingSession::SetRouter(IRouter * router)
{
  m_router.reset(router);
}

}
