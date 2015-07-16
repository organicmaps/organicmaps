#include "routing/routing_session.hpp"

#include "indexer/mercator.hpp"

#include "platform/location.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"

using namespace location;

namespace routing
{
static int const ON_ROUTE_MISSED_COUNT = 5;

RoutingSession::RoutingSession()
    : m_router(nullptr),
      m_route(string()),
      m_state(RoutingNotActive),
      m_endPoint(m2::PointD::Zero())
{
}

void RoutingSession::BuildRoute(m2::PointD const & startPoint, m2::PointD const & endPoint,
                                TReadyCallbackFn const & callback,
                                uint32_t timeoutSec)
{
  ASSERT(m_router != nullptr, ());
  m_lastGoodPosition = startPoint;
  m_endPoint = endPoint;
  m_router->ClearState();
  RebuildRoute(startPoint, callback, timeoutSec);
}

void RoutingSession::RebuildRoute(m2::PointD const & startPoint, TReadyCallbackFn const & callback,
                                  uint32_t timeoutSec)
{
  ASSERT(m_router != nullptr, ());
  ASSERT_NOT_EQUAL(m_endPoint, m2::PointD::Zero(), ("End point was not set"));
  RemoveRoute();
  m_state = RouteBuilding;

  ResetRoutingWatchdogTimer();

  // Use old-style callback construction, because lambda constructs buggy function on Android
  // (callback param isn't captured by value).
  m_router->CalculateRoute(startPoint, startPoint - m_lastGoodPosition, m_endPoint,
                           DoReadyCallback(*this, callback, m_routeSessionMutex));

  if (timeoutSec != 0)
    InitRoutingWatchdogTimer(timeoutSec);
}

void RoutingSession::DoReadyCallback::operator()(Route & route, IRouter::ResultCode e)
{
  threads::MutexGuard guard(m_routeSessionMutexInner);
  UNUSED_VALUE(guard);

  m_rs.AssignRoute(route);
  if (e != IRouter::NoError)
    m_rs.m_state = RouteNotReady;

  m_callback(m_rs.m_route, e);
}

void RoutingSession::RemoveRouteImpl()
{
  m_state = RoutingNotActive;
  m_lastDistance = 0.0;
  m_moveAwayCounter = 0;

  Route(string()).Swap(m_route);
}

void RoutingSession::RemoveRoute()
{
  threads::MutexGuard guard(m_routeSessionMutex);
  UNUSED_VALUE(guard);

  RemoveRouteImpl();
}

void RoutingSession::Reset()
{
  threads::MutexGuard guard(m_routeSessionMutex);
  UNUSED_VALUE(guard);

  RemoveRouteImpl();
  m_router->ClearState();
  m_turnsSound.Reset();

  ResetRoutingWatchdogTimer();
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

  if (m_state == RouteNeedRebuild || m_state == RouteFinished || m_state == RouteBuilding)
    return m_state;

  threads::MutexGuard guard(m_routeSessionMutex);
  UNUSED_VALUE(guard);
  ASSERT(m_route.IsValid(), ());

  m_turnsSound.SetSpeedMetersPerSecond(info.m_speed);

  if (m_route.MoveIterator(info))
  {
    m_moveAwayCounter = 0;
    m_lastDistance = 0.0;

    if (m_route.IsCurrentOnEnd())
      m_state = RouteFinished;
    else
      m_state = OnRoute;
    m_lastGoodPosition = position;
  }
  else
  {
    // Distance from the last known projection on route
    // (check if we are moving far from the last known projection).
    double const dist = m_route.GetCurrentSqDistance(position);
    if (dist > m_lastDistance || my::AlmostEqualULPs(dist, m_lastDistance, 1 << 16))
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

void RoutingSession::GetRouteFollowingInfo(FollowingInfo & info)
{
  auto formatDistFn = [](double dist, string & value, string & suffix)
  {
    /// @todo Make better formatting of distance and units.
    MeasurementUtils::FormatDistance(dist, value);

    size_t const delim = value.find(' ');
    ASSERT(delim != string::npos, ());
    suffix = value.substr(delim + 1);
    value.erase(delim);
  };
  // @todo(vbykoianko) The distance should depend on the current speed.
  double const kShowLanesDistInMeters = 500.;

  threads::MutexGuard guard(m_routeSessionMutex);
  UNUSED_VALUE(guard);

  if (m_route.IsValid() && IsNavigable())
  {
    formatDistFn(m_route.GetCurrentDistanceToEnd(), info.m_distToTarget, info.m_targetUnitsSuffix);

    double distanceToTurnMeters = 0.;
    turns::TurnItem turn;
    m_route.GetTurn(distanceToTurnMeters, turn);

    formatDistFn(distanceToTurnMeters, info.m_distToTurn, info.m_turnUnitsSuffix);
    info.m_turn = turn.m_turn;
    info.m_exitNum = turn.m_exitNum;
    info.m_time = m_route.GetTime();
    info.m_targetName = turn.m_targetName;

    // Lane information.
    if (distanceToTurnMeters < kShowLanesDistInMeters)
    {
      // There are two nested loops below. Outer one is for lanes and inner one (ctor of
      // SingleLaneInfo) is
      // for each lane's directions. The size of turn.m_lanes is relatively small. Less than 10 in
      // most cases.
      info.m_lanes.clear();
      for (size_t j = 0; j < turn.m_lanes.size(); ++j)
        info.m_lanes.emplace_back(turn.m_lanes[j]);
    }
    else
    {
      info.m_lanes.clear();
    }

    // Voice turn notifications.
    m_turnsSound.UpdateRouteFollowingInfo(info, turn, distanceToTurnMeters);
  }
  else
  {
    // nothing should be displayed on the screen about turns if these lines are executed
    info.m_turn = turns::TurnDirection::NoTurn;
    info.m_exitNum = 0;
    info.m_time = 0;
    info.m_targetName.clear();
    info.m_lanes.clear();
  }
}

void RoutingSession::AssignRoute(Route & route)
{
  if (route.IsValid())
    m_state = RouteNotStarted;
  else
    m_state = RoutingNotActive;

  route.SetRoutingSettings(m_routingSettings);
  m_route.Swap(route);
}

void RoutingSession::SetRouter(unique_ptr<IRouter> && router,
                               unique_ptr<OnlineAbsentCountriesFetcher> && fetcher,
                               TRoutingStatisticsCallback const & routingStatisticsFn)
{
  if (m_router)
    Reset();

  m_router.reset(new AsyncRouter(move(router), move(fetcher), routingStatisticsFn));
}

void RoutingSession::MatchLocationToRoute(location::GpsInfo & location,
                                          location::RouteMatchingInfo & routeMatchingInfo) const
{
  if (m_state != State::OnRoute)
    return;

  threads::MutexGuard guard(m_routeSessionMutex);
  UNUSED_VALUE(guard);
  m_route.MatchLocationToRoute(location, routeMatchingInfo);
}

void RoutingSession::SetRoutingSettings(RoutingSettings const & routingSettings)
{
  threads::MutexGuard guard(m_routeSessionMutex);
  UNUSED_VALUE(guard);
  m_routingSettings = routingSettings;
}

void RoutingSession::EnableTurnNotifications(bool enable)
{
  threads::MutexGuard guard(m_routeSessionMutex);
  UNUSED_VALUE(guard);
  m_turnsSound.Enable(enable);
}

bool RoutingSession::AreTurnNotificationsEnabled() const
{
  threads::MutexGuard guard(m_routeSessionMutex);
  UNUSED_VALUE(guard);
  return m_turnsSound.IsEnabled();
}

void RoutingSession::SetTurnSoundNotificationsUnits(routing::turns::sound::LengthUnits const & units)
{
  threads::MutexGuard guard(m_routeSessionMutex);
  UNUSED_VALUE(guard);
  m_turnsSound.SetLengthUnits(units);
}

routing::turns::sound::LengthUnits RoutingSession::GetTurnSoundNotificationsUnits() const
{
  threads::MutexGuard guard(m_routeSessionMutex);
  UNUSED_VALUE(guard);
  return m_turnsSound.GetLengthUnits();
}

void RoutingSession::ResetRoutingWatchdogTimer()
{
  if (m_routingWatchdog)
  {
    m_routingWatchdog->Cancel();
    m_routingWatchdog->WaitForCompletion();
    m_routingWatchdog.reset();
  }
}

void RoutingSession::InitRoutingWatchdogTimer(uint32_t timeoutSec)
{
  ASSERT_NOT_EQUAL(0, timeoutSec, ());
  ASSERT(nullptr == m_routingWatchdog, ());

  m_routingWatchdog = make_unique<DeferredTask>([this](){ m_router->ClearState(); }, seconds(timeoutSec));
}

}  // namespace routing
