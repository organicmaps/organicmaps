#include "routing/routing_session.hpp"

#include "routing/speed_camera.hpp"

#include "geometry/mercator.hpp"

#include "platform/location.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"

#include "std/utility.hpp"

#include "3party/Alohalytics/src/alohalytics.h"

using namespace location;
using namespace traffic;

namespace
{

int constexpr kOnRouteMissedCount = 5;

// @TODO(vbykoianko) The distance should depend on the current speed.
double constexpr kShowLanesDistInMeters = 500.;

// @todo(kshalnev) The distance may depend on the current speed.
double constexpr kShowPedestrianTurnInMeters = 5.;

double constexpr kRunawayDistanceSensitivityMeters = 0.01;

// Minimal distance to speed camera to make sound bell on overspeed.
double constexpr kSpeedCameraMinimalWarningMeters = 200.;
// Seconds to warning user before speed camera for driving with current speed.
double constexpr kSpeedCameraWarningSeconds = 30;

double constexpr kKmHToMps = 1000. / 3600.;

double constexpr kInvalidSpeedCameraDistance = -1;

// It limits depth of a speed camera point lookup along the route to avoid freezing.
size_t constexpr kSpeedCameraLookAheadCount = 50;

double constexpr kCompletionPercentAccuracy = 5;

double constexpr kMinimumETASec = 60.0;
}  // namespace

namespace routing
{
void FormatDistance(double dist, string & value, string & suffix)
{
  /// @todo Make better formatting of distance and units.
  UNUSED_VALUE(measurement_utils::FormatDistance(dist, value));

  size_t const delim = value.find(' ');
  ASSERT(delim != string::npos, ());
  suffix = value.substr(delim + 1);
  value.erase(delim);
};

RoutingSession::RoutingSession()
  : m_router(nullptr)
  , m_route(make_shared<Route>(string()))
  , m_state(RoutingNotActive)
  , m_isFollowing(false)
  , m_lastWarnedSpeedCameraIndex(0)
  , m_lastCheckedSpeedCameraIndex(0)
  , m_speedWarningSignal(false)
  , m_passedDistanceOnRouteMeters(0.0)
  , m_lastCompletionPercent(0.0)
{
}

void RoutingSession::Init(TRoutingStatisticsCallback const & routingStatisticsFn,
                          RouterDelegate::TPointCheckCallback const & pointCheckCallback)
{
  threads::MutexGuard guard(m_routingSessionMutex);
  ASSERT(m_router == nullptr, ());
  m_router.reset(new AsyncRouter(routingStatisticsFn, pointCheckCallback));
}

void RoutingSession::BuildRoute(Checkpoints const & checkpoints,
                                uint32_t timeoutSec)
{
  {
    threads::MutexGuard guard(m_routingSessionMutex);
    ASSERT(m_router != nullptr, ());
    m_checkpoints = checkpoints;
    m_router->ClearState();
    m_isFollowing = false;
    m_routingRebuildCount = -1; // -1 for the first rebuild.
  }

  RebuildRoute(checkpoints.GetStart(), m_buildReadyCallback, timeoutSec, RouteBuilding,
               false /* adjust */);
}

void RoutingSession::RebuildRoute(m2::PointD const & startPoint,
                                  TReadyCallback const & readyCallback, uint32_t timeoutSec,
                                  State routeRebuildingState, bool adjustToPrevRoute)
{
  {
    // @TODO(bykoianko) After moving all routing callbacks to single thread this guard can be
    // removed.
    threads::MutexGuard guard(m_routingSessionMutex);

    CHECK(m_router, ());
    RemoveRoute();
    SetState(routeRebuildingState);

    ++m_routingRebuildCount;
    m_lastCompletionPercent = 0;
    m_checkpoints.SetPointFrom(startPoint);
  }

  // Use old-style callback construction, because lambda constructs buggy function on Android
  // (callback param isn't captured by value).
  m_router->CalculateRoute(m_checkpoints, m_currentDirection, adjustToPrevRoute,
                           DoReadyCallback(*this, readyCallback, m_routingSessionMutex),
                           m_progressCallback, timeoutSec);
}

m2::PointD RoutingSession::GetStartPoint() const
{
  threads::MutexGuard guard(m_routingSessionMutex);
  return m_checkpoints.GetStart();
}

m2::PointD RoutingSession::GetEndPoint() const
{
  threads::MutexGuard guard(m_routingSessionMutex);
  return m_checkpoints.GetFinish();
}

void RoutingSession::DoReadyCallback::operator()(Route & route, IRouter::ResultCode e)
{
  threads::MutexGuard guard(m_routeSessionMutexInner);

  ASSERT(m_rs.m_route, ());

  if (e != IRouter::NeedMoreMaps)
  {
    m_rs.AssignRoute(route, e);
  }
  else
  {
    for (string const & country : route.GetAbsentCountries())
      m_rs.m_route->AddAbsentCountry(country);
  }

  m_callback(*m_rs.m_route, e);
}

void RoutingSession::RemoveRoute()
{
  SetState(RoutingNotActive);
  m_lastDistance = 0.0;
  m_moveAwayCounter = 0;
  m_turnNotificationsMgr.Reset();

  m_route = make_shared<Route>(string());
}

void RoutingSession::RebuildRouteOnTrafficUpdate()
{
  m2::PointD startPoint;

  {
    // @TODO(bykoianko) After moving all routing callbacks to single thread this guard can be
    // removed.
    threads::MutexGuard guard(m_routingSessionMutex);
    startPoint = m_lastGoodPosition;

    switch (m_state.load())
    {
    case RoutingNotActive:
    case RouteNotReady:
    case RouteFinished: return;

    case RouteBuilding:
    case RouteNotStarted:
    case RouteNoFollowing:
    case RouteRebuilding: startPoint = m_checkpoints.GetPointFrom();

    case OnRoute:
    case RouteNeedRebuild: break;
    }

    // Cancel current route building.
    m_router->ClearState();
  }

  RebuildRoute(startPoint, m_rebuildReadyCallback, 0 /* timeoutSec */,
               routing::RoutingSession::State::RouteRebuilding, false /* adjustToPrevRoute */);
}

void RoutingSession::Reset()
{
  threads::MutexGuard guard(m_routingSessionMutex);
  ResetImpl();
}

void RoutingSession::ResetImpl()
{
  ASSERT(m_router != nullptr, ());

  RemoveRoute();
  m_router->ClearState();

  m_passedDistanceOnRouteMeters = 0.0;
  m_lastWarnedSpeedCameraIndex = 0;
  m_lastCheckedSpeedCameraIndex = 0;
  m_lastFoundCamera = SpeedCameraRestriction();
  m_speedWarningSignal = false;
  m_isFollowing = false;
  m_lastCompletionPercent = 0;
}

RoutingSession::State RoutingSession::OnLocationPositionChanged(GpsInfo const & info, Index const & index)
 {
  ASSERT(m_state != RoutingNotActive, ());
  ASSERT(m_router != nullptr, ());

  if (m_state == RouteNeedRebuild || m_state == RouteFinished
      || m_state == RouteBuilding || m_state == RouteRebuilding
      || m_state == RouteNotReady || m_state == RouteNoFollowing)
    return m_state;

  threads::MutexGuard guard(m_routingSessionMutex);
  ASSERT(m_route, ());
  ASSERT(m_route->IsValid(), ());

  m_turnNotificationsMgr.SetSpeedMetersPerSecond(info.m_speed);

  if (m_route->MoveIterator(info))
  {
    m_moveAwayCounter = 0;
    m_lastDistance = 0.0;

    PassCheckpoints();

    if (m_checkpoints.IsFinished())
    {
      m_passedDistanceOnRouteMeters += m_route->GetTotalDistanceMeters();
      SetState(RouteFinished);

      alohalytics::TStringMap params = {{"router", m_route->GetRouterId()},
                                        {"passedDistance", strings::to_string(m_passedDistanceOnRouteMeters)},
                                        {"rebuildCount", strings::to_string(m_routingRebuildCount)}};
      alohalytics::LogEvent("RouteTracking_ReachedDestination", params);
    }
    else
    {
      SetState(OnRoute);

      // Warning signals checks
      if (m_routingSettings.m_speedCameraWarning && !m_speedWarningSignal)
      {
        double const warningDistanceM = max(kSpeedCameraMinimalWarningMeters,
                                            info.m_speed * kSpeedCameraWarningSeconds);
        SpeedCameraRestriction cam(0, 0);
        double const camDistance = GetDistanceToCurrentCamM(cam, index);
        if (kInvalidSpeedCameraDistance != camDistance && camDistance < warningDistanceM)
        {
          if (cam.m_index > m_lastWarnedSpeedCameraIndex && info.m_speed > cam.m_maxSpeedKmH * kKmHToMps)
          {
            m_speedWarningSignal = true;
            m_lastWarnedSpeedCameraIndex = cam.m_index;
          }
        }
      }
    }

    if (m_userCurrentPositionValid)
      m_lastGoodPosition = m_userCurrentPosition;
  }
  else
  {
    // Distance from the last known projection on route
    // (check if we are moving far from the last known projection).
    auto const & lastGoodPoint = m_route->GetFollowedPolyline().GetCurrentIter().m_pt;
    double const dist = MercatorBounds::DistanceOnEarth(lastGoodPoint,
                                                        MercatorBounds::FromLatLon(info.m_latitude, info.m_longitude));
    if (my::AlmostEqualAbs(dist, m_lastDistance, kRunawayDistanceSensitivityMeters))
        return m_state;
    if (dist > m_lastDistance)
    {
      ++m_moveAwayCounter;
      m_lastDistance = dist;
    }

    if (m_moveAwayCounter > kOnRouteMissedCount)
    {
      m_passedDistanceOnRouteMeters += m_route->GetCurrentDistanceFromBeginMeters();
      SetState(RouteNeedRebuild);
      alohalytics::TStringMap params = {
          {"router", m_route->GetRouterId()},
          {"percent", strings::to_string(GetCompletionPercent())},
          {"passedDistance", strings::to_string(m_passedDistanceOnRouteMeters)},
          {"rebuildCount", strings::to_string(m_routingRebuildCount)}};
      alohalytics::LogEvent(
          "RouteTracking_RouteNeedRebuild", params,
          alohalytics::Location::FromLatLon(MercatorBounds::YToLat(lastGoodPoint.y),
                                            MercatorBounds::XToLon(lastGoodPoint.x)));
    }
  }

  if (m_userCurrentPositionValid && m_userFormerPositionValid)
    m_currentDirection = m_userCurrentPosition - m_userFormerPosition;

  return m_state;
}

void RoutingSession::GetRouteFollowingInfo(FollowingInfo & info) const
{
  threads::MutexGuard guard(m_routingSessionMutex);

  ASSERT(m_route, ());

  if (!m_route->IsValid())
  {
    // nothing should be displayed on the screen about turns if these lines are executed
    info = FollowingInfo();
    return;
  }

  if (!IsNavigable())
  {
    info = FollowingInfo();
    FormatDistance(m_route->GetTotalDistanceMeters(), info.m_distToTarget, info.m_targetUnitsSuffix);
    info.m_time = static_cast<int>(max(kMinimumETASec, m_route->GetCurrentTimeToEndSec()));
    return;
  }

  FormatDistance(m_route->GetCurrentDistanceToEndMeters(), info.m_distToTarget, info.m_targetUnitsSuffix);

  double distanceToTurnMeters = 0.;
  turns::TurnItem turn;
  m_route->GetCurrentTurn(distanceToTurnMeters, turn);
  FormatDistance(distanceToTurnMeters, info.m_distToTurn, info.m_turnUnitsSuffix);
  info.m_turn = turn.m_turn;

  // The turn after the next one.
  if (m_routingSettings.m_showTurnAfterNext)
    info.m_nextTurn = m_turnNotificationsMgr.GetSecondTurnNotification();
  else
    info.m_nextTurn = routing::turns::CarDirection::None;

  info.m_exitNum = turn.m_exitNum;
  info.m_time = static_cast<int>(max(kMinimumETASec, m_route->GetCurrentTimeToEndSec()));
  m_route->GetCurrentStreetName(info.m_sourceName);
  m_route->GetStreetNameAfterIdx(turn.m_index, info.m_targetName);
  info.m_completionPercent = GetCompletionPercent();

  // Lane information and next street name.
  if (distanceToTurnMeters < kShowLanesDistInMeters)
  {
    info.m_displayedStreetName = info.m_targetName;
    // There are two nested loops below. Outer one is for lanes and inner one (ctor of
    // SingleLaneInfo) is
    // for each lane's directions. The size of turn.m_lanes is relatively small. Less than 10 in
    // most cases.
    info.m_lanes.clear();
    info.m_lanes.reserve(turn.m_lanes.size());
    for (size_t j = 0; j < turn.m_lanes.size(); ++j)
      info.m_lanes.emplace_back(turn.m_lanes[j]);
  }
  else
  {
    info.m_displayedStreetName = "";
    info.m_lanes.clear();
  }

  // Speedcam signal information.
  info.m_speedWarningSignal = m_speedWarningSignal;
  m_speedWarningSignal = false;

  // Pedestrian info
  m2::PointD pos;
  m_route->GetCurrentDirectionPoint(pos);
  info.m_pedestrianDirectionPos = MercatorBounds::ToLatLon(pos);
  info.m_pedestrianTurn =
      (distanceToTurnMeters < kShowPedestrianTurnInMeters) ? turn.m_pedestrianTurn : turns::PedestrianDirection::None;
}

double RoutingSession::GetCompletionPercent() const
{
  ASSERT(m_route, ());

  double const denominator = m_passedDistanceOnRouteMeters + m_route->GetTotalDistanceMeters();
  if (!m_route->IsValid() || denominator == 0.0)
    return 0;

  double const percent = 100.0 *
    (m_passedDistanceOnRouteMeters + m_route->GetCurrentDistanceFromBeginMeters()) /
    denominator;
  if (percent - m_lastCompletionPercent > kCompletionPercentAccuracy)
  {
    auto const lastGoodPoint =
        MercatorBounds::ToLatLon(m_route->GetFollowedPolyline().GetCurrentIter().m_pt);
    alohalytics::Stats::Instance().LogEvent(
        "RouteTracking_PercentUpdate", {{"percent", strings::to_string(percent)}},
        alohalytics::Location::FromLatLon(lastGoodPoint.lat, lastGoodPoint.lon));
    m_lastCompletionPercent = percent;
  }
  return percent;
}

void RoutingSession::PassCheckpoints()
{
  while (!m_checkpoints.IsFinished() && m_route->IsSubroutePassed(m_checkpoints.GetPassedIdx()))
  {
    m_route->PassNextSubroute();
    m_checkpoints.PassNextPoint();
    LOG(LINFO, ("Pass checkpoint, ", m_checkpoints));
    m_checkpointCallback(m_checkpoints.GetPassedIdx());
  }
}

void RoutingSession::GenerateTurnNotifications(vector<string> & turnNotifications)
{
  turnNotifications.clear();

  threads::MutexGuard guard(m_routingSessionMutex);

  ASSERT(m_route, ());

  // Voice turn notifications.
  if (!m_routingSettings.m_soundDirection)
    return;

  if (!m_route->IsValid() || !IsNavigable())
    return;

  vector<turns::TurnItemDist> turns;
  if (m_route->GetNextTurns(turns))
    m_turnNotificationsMgr.GenerateTurnNotifications(turns, turnNotifications);
}

void RoutingSession::AssignRoute(Route & route, IRouter::ResultCode e)
{
  if (e != IRouter::Cancelled)
  {
    if (route.IsValid())
      SetState(RouteNotStarted);
    else
      SetState(RoutingNotActive);

    if (e != IRouter::NoError)
      SetState(RouteNotReady);
  }
  else
  {
    SetState(RoutingNotActive);
  }

  ASSERT(m_route, ());

  route.SetRoutingSettings(m_routingSettings);
  m_route->Swap(route);
  m_lastWarnedSpeedCameraIndex = 0;
  m_lastCheckedSpeedCameraIndex = 0;
  m_lastFoundCamera = SpeedCameraRestriction();
}

void RoutingSession::SetRouter(unique_ptr<IRouter> && router,
                               unique_ptr<OnlineAbsentCountriesFetcher> && fetcher)
{
  threads::MutexGuard guard(m_routingSessionMutex);
  ASSERT(m_router != nullptr, ());
  ResetImpl();
  m_router->SetRouter(move(router), move(fetcher));
}

void RoutingSession::MatchLocationToRoute(location::GpsInfo & location,
                                          location::RouteMatchingInfo & routeMatchingInfo) const
{
  if (!IsOnRoute())
    return;

  threads::MutexGuard guard(m_routingSessionMutex);

  ASSERT(m_route, ());

  m_route->MatchLocationToRoute(location, routeMatchingInfo);
}

traffic::SpeedGroup RoutingSession::MatchTraffic(
    location::RouteMatchingInfo const & routeMatchingInfo) const
{
  if (!routeMatchingInfo.IsMatched())
    return SpeedGroup::Unknown;

  size_t const index = routeMatchingInfo.GetIndexInRoute();
  threads::MutexGuard guard(m_routingSessionMutex);

  return m_route->GetTraffic(index);
}

bool RoutingSession::DisableFollowMode()
{
  LOG(LINFO, ("Routing disables a following mode. State: ", m_state.load()));
  if (m_state == RouteNotStarted || m_state == OnRoute)
  {
    SetState(RouteNoFollowing);
    m_isFollowing = false;
    return true;
  }
  return false;
}

bool RoutingSession::EnableFollowMode()
{
  LOG(LINFO, ("Routing enables a following mode. State: ", m_state.load()));
  if (m_state == RouteNotStarted || m_state == OnRoute)
  {
    SetState(OnRoute);
    m_isFollowing = true;
  }
  return m_isFollowing;
}

void RoutingSession::SetRoutingSettings(RoutingSettings const & routingSettings)
{
  threads::MutexGuard guard(m_routingSessionMutex);
  m_routingSettings = routingSettings;
}

void RoutingSession::SetReadyCallbacks(TReadyCallback const & buildReadyCallback,
                                       TReadyCallback const & rebuildReadyCallback)
{
  m_buildReadyCallback = buildReadyCallback;
  // m_rebuildReadyCallback used from multiple threads but it's the only place we write m_rebuildReadyCallback
  // and this method is called from RoutingManager constructor before it can be used from any other place.
  // We can use mutex
  //   1) here to protect m_rebuildReadyCallback
  //   2) and inside BuldRoute & RebuildRouteOnTrafficUpdate to safely copy m_rebuildReadyCallback to temporary
  //      variable cause we need pass it to RebuildRoute and do not want to execute route rebuild with mutex.
  // But it'll make code worse and will not improve safety.
  m_rebuildReadyCallback = rebuildReadyCallback;
}

void RoutingSession::SetProgressCallback(TProgressCallback const & progressCallback)
{
  m_progressCallback = progressCallback;
}

void RoutingSession::SetCheckpointCallback(CheckpointCallback const & checkpointCallback)
{
  m_checkpointCallback = checkpointCallback;
}

void RoutingSession::SetUserCurrentPosition(m2::PointD const & position)
{
  // All methods which read/write m_userCurrentPosition*, m_userFormerPosition*  work in RoutingManager thread
  m_userFormerPosition = m_userCurrentPosition;
  m_userFormerPositionValid = m_userCurrentPositionValid;

  m_userCurrentPosition = position;
  m_userCurrentPositionValid = true;
}

void RoutingSession::EnableTurnNotifications(bool enable)
{
  threads::MutexGuard guard(m_routingSessionMutex);
  m_turnNotificationsMgr.Enable(enable);
}

bool RoutingSession::AreTurnNotificationsEnabled() const
{
  threads::MutexGuard guard(m_routingSessionMutex);
  return m_turnNotificationsMgr.IsEnabled();
}

void RoutingSession::SetTurnNotificationsUnits(measurement_utils::Units const units)
{
  threads::MutexGuard guard(m_routingSessionMutex);
  m_turnNotificationsMgr.SetLengthUnits(units);
}

void RoutingSession::SetTurnNotificationsLocale(string const & locale)
{
  LOG(LINFO, ("The language for turn notifications is", locale));
  threads::MutexGuard guard(m_routingSessionMutex);
  m_turnNotificationsMgr.SetLocale(locale);
}

string RoutingSession::GetTurnNotificationsLocale() const
{
  threads::MutexGuard guard(m_routingSessionMutex);
  return m_turnNotificationsMgr.GetLocale();
}

double RoutingSession::GetDistanceToCurrentCamM(SpeedCameraRestriction & camera, Index const & index)
{
  ASSERT(m_route, ());

  auto const & m_poly = m_route->GetFollowedPolyline();
  auto const & currentIter = m_poly.GetCurrentIter();
  if (currentIter.m_ind < m_lastFoundCamera.m_index &&
      m_lastFoundCamera.m_index < m_poly.GetPolyline().GetSize())
  {
    camera = m_lastFoundCamera;
    return m_poly.GetDistanceM(currentIter, m_poly.GetIterToIndex(camera.m_index));
  }
  size_t const currentIndex = max(currentIter.m_ind, m_lastCheckedSpeedCameraIndex + 1);
  size_t const upperBound = min(m_poly.GetPolyline().GetSize(), currentIndex + kSpeedCameraLookAheadCount);
  for (m_lastCheckedSpeedCameraIndex = currentIndex; m_lastCheckedSpeedCameraIndex < upperBound; ++m_lastCheckedSpeedCameraIndex)
  {
    uint8_t speed = CheckCameraInPoint(m_poly.GetPolyline().GetPoint(m_lastCheckedSpeedCameraIndex), index);
    if (speed != kNoSpeedCamera)
    {
      camera = SpeedCameraRestriction(static_cast<uint32_t>(m_lastCheckedSpeedCameraIndex), speed);
      m_lastFoundCamera = camera;
      return m_poly.GetDistanceM(currentIter, m_poly.GetIterToIndex(m_lastCheckedSpeedCameraIndex));
    }
  }
  return kInvalidSpeedCameraDistance;
}

void RoutingSession::ProtectedCall(RouteCallback const & callback) const
{
  threads::MutexGuard guard(m_routingSessionMutex);
  CHECK(m_route, ());
  callback(*m_route);
}

void RoutingSession::EmitCloseRoutingEvent() const
{
  threads::MutexGuard guard(m_routingSessionMutex);
  ASSERT(m_route, ());

  if (!m_route->IsValid())
  {
    ASSERT(false, ());
    return;
  }
  auto const lastGoodPoint =
      MercatorBounds::ToLatLon(m_route->GetFollowedPolyline().GetCurrentIter().m_pt);
  alohalytics::Stats::Instance().LogEvent(
      "RouteTracking_RouteClosing",
      {{"percent", strings::to_string(GetCompletionPercent())},
       {"distance", strings::to_string(m_passedDistanceOnRouteMeters +
                                       m_route->GetCurrentDistanceToEndMeters())},
       {"router", m_route->GetRouterId()},
       {"rebuildCount", strings::to_string(m_routingRebuildCount)}},
      alohalytics::Location::FromLatLon(lastGoodPoint.lat, lastGoodPoint.lon));
}

bool RoutingSession::HasRouteAltitude() const
{
  threads::MutexGuard guard(m_routingSessionMutex);
  ASSERT(m_route, ());
  return m_route->HaveAltitudes();
}

bool RoutingSession::GetRouteAltitudesAndDistancesM(vector<double> & routeSegDistanceM,
                                                    feature::TAltitudes & routeAltitudesM) const
{
  threads::MutexGuard guard(m_routingSessionMutex);
  ASSERT(m_route, ());

  if (!m_route->IsValid() || !m_route->HaveAltitudes())
    return false;

  routeSegDistanceM = m_route->GetSegDistanceMeters();
  feature::TAltitudes altitudes;
  m_route->GetAltitudes(routeAltitudesM);
  return true;
}

void RoutingSession::OnTrafficInfoClear()
{
  {
    threads::MutexGuard guard(m_routingSessionMutex);
    Clear();
  }
  RebuildRouteOnTrafficUpdate();
}

void RoutingSession::OnTrafficInfoAdded(TrafficInfo && info)
{
  TrafficInfo::Coloring const & fullColoring = info.GetColoring();
  TrafficInfo::Coloring coloring;
  for (auto const & kv : fullColoring)
  {
    ASSERT_NOT_EQUAL(kv.second, SpeedGroup::Unknown, ());
    coloring.insert(kv);
  }

  {
    threads::MutexGuard guard(m_routingSessionMutex);
    Set(info.GetMwmId(), move(coloring));
  }
  RebuildRouteOnTrafficUpdate();
}

void RoutingSession::OnTrafficInfoRemoved(MwmSet::MwmId const & mwmId)
{
  {
    threads::MutexGuard guard(m_routingSessionMutex);
    Remove(mwmId);
  }
  RebuildRouteOnTrafficUpdate();
}

shared_ptr<TrafficInfo::Coloring> RoutingSession::GetTrafficInfo(MwmSet::MwmId const & mwmId) const
{
  threads::MutexGuard guard(m_routingSessionMutex);
  return TrafficCache::GetTrafficInfo(mwmId);
}

void RoutingSession::CopyTraffic(std::map<MwmSet::MwmId, std::shared_ptr<traffic::TrafficInfo::Coloring>> & trafficColoring) const
{
  threads::MutexGuard guard(m_routingSessionMutex);
  TrafficCache::CopyTraffic(trafficColoring);
}

string DebugPrint(RoutingSession::State state)
{
  switch (state)
  {
  case RoutingSession::RoutingNotActive: return "RoutingNotActive";
  case RoutingSession::RouteBuilding: return "RouteBuilding";
  case RoutingSession::RouteNotReady: return "RouteNotReady";
  case RoutingSession::RouteNotStarted: return "RouteNotStarted";
  case RoutingSession::OnRoute: return "OnRoute";
  case RoutingSession::RouteNeedRebuild: return "RouteNeedRebuild";
  case RoutingSession::RouteFinished: return "RouteFinished";
  case RoutingSession::RouteNoFollowing: return "RouteNoFollowing";
  case RoutingSession::RouteRebuilding: return "RouteRebuilding";
  }
}
}  // namespace routing
