#include "routing/routing_session.hpp"

#include "indexer/mercator.hpp"

#include "platform/location.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"

using namespace location;

namespace
{

int constexpr kOnRouteMissedCount = 5;

// @todo(vbykoianko) The distance should depend on the current speed.
double constexpr kShowLanesDistInMeters = 500.;

// @todo(kshalnev) The distance may depend on the current speed.
double constexpr kShowPedestrianTurnInMeters = 5.;

}  // namespace

namespace routing
{

RoutingSession::RoutingSession()
    : m_router(nullptr),
      m_route(string()),
      m_state(RoutingNotActive),
      m_endPoint(m2::PointD::Zero())
{
}

void RoutingSession::Init(TRoutingStatisticsCallback const & routingStatisticsFn,
                          RouterDelegate::TPointCheckCallback const & pointCheckCallback)
{
  ASSERT(m_router == nullptr, ());
  m_router.reset(new AsyncRouter(routingStatisticsFn, pointCheckCallback));
}

void RoutingSession::BuildRoute(m2::PointD const & startPoint, m2::PointD const & endPoint,
                                TReadyCallback const & readyCallback,
                                TProgressCallback const & progressCallback,
                                uint32_t timeoutSec)
{
  ASSERT(m_router != nullptr, ());
  m_lastGoodPosition = startPoint;
  m_endPoint = endPoint;
  m_router->ClearState();
  RebuildRoute(startPoint, readyCallback, progressCallback, timeoutSec);
}

void RoutingSession::RebuildRoute(m2::PointD const & startPoint,
    TReadyCallback const & readyCallback,
    TProgressCallback const & progressCallback, uint32_t timeoutSec)
{
  ASSERT(m_router != nullptr, ());
  ASSERT_NOT_EQUAL(m_endPoint, m2::PointD::Zero(), ("End point was not set"));
  RemoveRoute();
  m_state = RouteBuilding;

  // Use old-style callback construction, because lambda constructs buggy function on Android
  // (callback param isn't captured by value).
  m_router->CalculateRoute(startPoint, startPoint - m_lastGoodPosition, m_endPoint,
                           DoReadyCallback(*this, readyCallback, m_routeSessionMutex),
                           progressCallback, timeoutSec);
}

void RoutingSession::DoReadyCallback::operator()(Route & route, IRouter::ResultCode e)
{
  threads::MutexGuard guard(m_routeSessionMutexInner);
  UNUSED_VALUE(guard);

  m_rs.AssignRoute(route, e);

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
  ASSERT(m_router != nullptr, ());

  threads::MutexGuard guard(m_routeSessionMutex);
  UNUSED_VALUE(guard);

  RemoveRouteImpl();
  m_router->ClearState();
  m_turnsSound.Reset();
}

RoutingSession::State RoutingSession::OnLocationPositionChanged(m2::PointD const & position,
                                                                GpsInfo const & info)
{
  ASSERT(m_state != RoutingNotActive, ());
  ASSERT(m_router != nullptr, ());

  if (m_state == RouteNotReady)
  {
    if (++m_moveAwayCounter > kOnRouteMissedCount)
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

    if (m_moveAwayCounter > kOnRouteMissedCount)
      m_state = RouteNeedRebuild;
  }

  return m_state;
}

void RoutingSession::GetRouteFollowingInfo(FollowingInfo & info) const
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

  threads::MutexGuard guard(m_routeSessionMutex);
  UNUSED_VALUE(guard);

  if (m_route.IsValid() && IsNavigable())
  {
    formatDistFn(m_route.GetCurrentDistanceToEndMeters(), info.m_distToTarget, info.m_targetUnitsSuffix);

    double distanceToTurnMeters = 0.;
    turns::TurnItem turn;
    m_route.GetCurrentTurn(distanceToTurnMeters, turn);

    formatDistFn(distanceToTurnMeters, info.m_distToTurn, info.m_turnUnitsSuffix);
    info.m_turn = turn.m_turn;
    info.m_exitNum = turn.m_exitNum;
    info.m_time = m_route.GetCurrentTimeToEndSec();
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

    // Pedestrian info
    m2::PointD pos;
    m_route.GetCurrentDirectionPoint(pos);
    info.m_pedestrianDirectionPos = MercatorBounds::ToLatLon(pos);
    info.m_pedestrianTurn =
        (distanceToTurnMeters < kShowPedestrianTurnInMeters) ? turn.m_pedestrianTurn : turns::PedestrianDirection::None;
  }
  else
  {
    // nothing should be displayed on the screen about turns if these lines are executed
    info = FollowingInfo();
  }
}

void RoutingSession::GenerateTurnSound(vector<string> & turnNotifications)
{
  turnNotifications.clear();

  threads::MutexGuard guard(m_routeSessionMutex);
  UNUSED_VALUE(guard);
  // Voice turn notifications.
  if (!m_routingSettings.m_soundDirection)
    return;

  if (!m_route.IsValid() || !IsNavigable())
    return;

  double distanceToTurnMeters = 0.;
  turns::TurnItem turn;
  m_route.GetCurrentTurn(distanceToTurnMeters, turn);

  m_turnsSound.GenerateTurnSound(turn, distanceToTurnMeters, turnNotifications);
}

void RoutingSession::AssignRoute(Route & route, IRouter::ResultCode e)
{
  if (e != IRouter::Cancelled)
  {
    if (route.IsValid())
      m_state = RouteNotStarted;
    else
      m_state = RoutingNotActive;

    if (e != IRouter::NoError)
      m_state = RouteNotReady;
  }
  else
    m_state = RoutingNotActive;

  route.SetRoutingSettings(m_routingSettings);
  m_route.Swap(route);
}

void RoutingSession::SetRouter(unique_ptr<IRouter> && router,
                               unique_ptr<OnlineAbsentCountriesFetcher> && fetcher)
{
  ASSERT(m_router != nullptr, ());
  Reset();
  m_router->SetRouter(move(router), move(fetcher));
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

bool RoutingSession::GetMercatorDistanceFromBegin(double & distance) const
{
  if (m_state != State::OnRoute)
  {
    distance = 0.0;
    return false;
  }

  threads::MutexGuard guard(m_routeSessionMutex);
  UNUSED_VALUE(guard);

  distance = m_route.GetMercatorDistanceFromBegin();
  return true;
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

void RoutingSession::SetTurnNotificationsUnits(routing::turns::sound::LengthUnits const & units)
{
  threads::MutexGuard guard(m_routeSessionMutex);
  UNUSED_VALUE(guard);
  m_turnsSound.SetLengthUnits(units);
}

void RoutingSession::SetTurnNotificationsLocale(string const & locale)
{
  threads::MutexGuard guard(m_routeSessionMutex);
  UNUSED_VALUE(guard);
  m_turnsSound.SetLocale(locale);
}

string RoutingSession::GetTurnNotificationsLocale() const
{
  threads::MutexGuard guard(m_routeSessionMutex);
  UNUSED_VALUE(guard);
  return m_turnsSound.GetLocale();
}
}  // namespace routing
