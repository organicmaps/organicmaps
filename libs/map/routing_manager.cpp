#include "routing_manager.hpp"

#include "map/routing_mark.hpp"

#include "routing/absent_regions_finder.hpp"
#include "routing/checkpoint_predictor.hpp"
#include "routing/index_router.hpp"
#include "routing/route.hpp"
#include "routing/routing_callbacks.hpp"
#include "routing/routing_options.hpp"
#include "routing/ruler_router.hpp"
#include "routing/speed_camera.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/routing_helpers.hpp"

#include "indexer/classificator.hpp"
#include "indexer/data_source.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "platform/country_file.hpp"
#include "platform/distance.hpp"
#include "platform/duration.hpp"
#include "platform/platform.hpp"

#include "geometry/algorithm.hpp"
#include "geometry/angles.hpp"
#include "geometry/mercator.hpp"  // kPointEqualityEps
#include "geometry/parametrized_segment.hpp"

#include "coding/file_writer.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"
#include "base/scope_guard.hpp"
#include "base/small_map.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <glaze/json.hpp>

#include <algorithm>
#include <cmath>
#include <map>

using namespace routing;

namespace route_points_json
{
struct RoutePointJson
{
  int type = 0;
  std::string title;
  std::string subtitle;
  double x = 0.0;
  double y = 0.0;
  bool replaceWithMyPosition = false;
};

RoutePointJson ToRoutePointJson(RouteMarkData const & data)
{
  return {.type = static_cast<int>(data.m_pointType),
          .title = data.m_title,
          .subtitle = data.m_subTitle,
          .x = data.m_position.x,
          .y = data.m_position.y,
          .replaceWithMyPosition = data.m_replaceWithMyPositionAfterRestart};
}

RouteMarkData ToRouteMarkData(RoutePointJson const & point)
{
  RouteMarkData data;
  data.m_pointType = static_cast<RouteMarkType>(point.type);
  data.m_title = point.title;
  data.m_subTitle = point.subtitle;
  data.m_position = {point.x, point.y};
  data.m_replaceWithMyPositionAfterRestart = point.replaceWithMyPosition;
  return data;
}
}  // namespace route_points_json

namespace
{
std::string_view constexpr kRouterTypeKey = "router";

double constexpr kRouteScaleMultiplier = 1.5;

std::string const kRoutePointsFile = "route_points.dat";

uint32_t constexpr kInvalidTransactionId = 0;

void FillTurnsDistancesForRendering(std::vector<RouteSegment> const & segments, double baseDistance,
                                    std::vector<double> & turns)
{
  using namespace routing::turns;
  turns.clear();
  turns.reserve(segments.size());
  for (auto const & s : segments)
  {
    auto const & t = s.GetTurn();
    CHECK_NOT_EQUAL(t.m_turn, CarDirection::Count, ());
    // We do not render some of the turn directions.
    if (t.m_turn == CarDirection::None || t.m_turn == CarDirection::StartAtEndOfStreet ||
        t.m_turn == CarDirection::StayOnRoundAbout || t.m_turn == CarDirection::ReachedYourDestination)
    {
      continue;
    }
    turns.push_back(s.GetDistFromBeginningMerc() - baseDistance);
  }
}

void FillTrafficForRendering(std::vector<RouteSegment> const & segments, std::vector<traffic::SpeedGroup> & traffic)
{
  traffic.clear();
  traffic.reserve(segments.size());
  for (auto const & s : segments)
    traffic.push_back(s.GetTraffic());
}

RouteMarkData GetLastPassedPoint(BookmarkManager * bmManager, std::vector<RouteMarkData> const & points)
{
  ASSERT_GREATER_OR_EQUAL(points.size(), 2, ());
  ASSERT(points[0].m_pointType == RouteMarkType::Start, ());
  RouteMarkData data = points[0];

  for (int i = static_cast<int>(points.size()) - 1; i >= 0; i--)
  {
    if (points[i].m_isPassed)
    {
      data = points[i];
      break;
    }
  }

  // Last passed point will be considered as start point.
  data.m_pointType = RouteMarkType::Start;
  data.m_intermediateIndex = 0;
  if (data.m_isMyPosition)
  {
    data.m_position = bmManager->MyPositionMark().GetPivot();
    data.m_isMyPosition = false;
    data.m_replaceWithMyPositionAfterRestart = true;
  }

  return data;
}

std::string SerializeRoutePoints(std::vector<RouteMarkData> const & points)
{
  ASSERT_GREATER_OR_EQUAL(points.size(), 2, ());
  std::vector<route_points_json::RoutePointJson> pointsJson;
  pointsJson.reserve(points.size());
  for (auto const & p : points)
    pointsJson.push_back(route_points_json::ToRoutePointJson(p));

  std::string buffer;
  if (auto const error = glz::write_json(pointsJson, buffer); error)
    MYTHROW(RootException, (glz::format_error(error)));
  return buffer;
}

std::vector<RouteMarkData> DeserializeRoutePoints(std::string const & data)
{
  std::vector<route_points_json::RoutePointJson> pointsJson;
  glz::opts constexpr opts{.error_on_unknown_keys = false, .error_on_missing_keys = false};
  if (auto const error = glz::read<opts>(pointsJson, data); error || pointsJson.empty())
    return {};

  std::vector<RouteMarkData> result;
  result.reserve(pointsJson.size());
  for (auto const & pointJson : pointsJson)
  {
    auto point = route_points_json::ToRouteMarkData(pointJson);
    if (point.m_position.EqualDxDy(m2::PointD::Zero(), mercator::kPointEqualityEps))
      continue;

    result.push_back(std::move(point));
  }

  if (result.size() < 2)
    return {};

  return result;
}

// Maps a barrier point-feature classificator type (stored in routing::Route::GetWarnings) to a UI
// warning mark type. Mirrors ftypes::IsWayChecker; this is the single place to extend when adding
// a new barrier warning kind.
class BarrierWarningChecker : public ftypes::BaseChecker
{
public:
  BarrierWarningChecker()
  {
    Classificator const & c = classif();
    std::pair<char const *, RoadWarningMarkType> const types[] = {
        {"gate", RoadWarningMarkType::Gate},
        {"lift_gate", RoadWarningMarkType::LiftGate},
    };

    m_marks.Reserve(std::size(types));
    for (auto const & e : types)
    {
      uint32_t const type = c.GetTypeByPath({"barrier", e.first});
      m_types.push_back(type);
      m_marks.Insert(type, e.second);
    }
    m_marks.FinishBuilding();
  }

  DECLARE_CHECKER_INSTANCE(BarrierWarningChecker);

  /// @returns RoadWarningMarkType::Count if |type| is not a known barrier warning.
  RoadWarningMarkType GetWarningType(uint32_t type) const
  {
    if (auto const * res = m_marks.Find(ftype::Trunc(type, 2)))
      return *res;
    return RoadWarningMarkType::Count;
  }

private:
  base::SmallMap<uint32_t, RoadWarningMarkType> m_marks;
};

drape_ptr<df::Subroute> CreateDrapeSubroute(std::vector<RouteSegment> const & segments, m2::PointD const & startPt,
                                            double baseDistance, double baseDepth, routing::RouterType routerType)
{
  auto subroute = make_unique_dp<df::Subroute>();
  subroute->m_baseDistance = baseDistance;
  subroute->m_baseDepthIndex = baseDepth;

  auto constexpr kBias = 1.0;

  if (routerType == RouterType::Transit)
  {
    subroute->m_headFakeDistance = -kBias;
    subroute->m_tailFakeDistance = kBias;
    subroute->m_polyline.Add(startPt);
    return subroute;
  }

  std::vector<m2::PointD> points;
  points.reserve(segments.size() + 1);
  points.push_back(startPt);
  for (auto const & s : segments)
    points.push_back(s.GetJunction().GetPoint());

  if (points.size() < 2)
  {
    LOG(LWARNING, ("Invalid subroute. Points number =", points.size()));
    return nullptr;
  }

  if (routerType == RouterType::Ruler)
  {
    auto const subrouteLen = segments.back().GetDistFromBeginningMerc() - baseDistance;
    subroute->m_headFakeDistance = -kBias;
    subroute->m_tailFakeDistance = subrouteLen + kBias;
    subroute->m_polyline = m2::PolylineD(std::move(points));
    return subroute;
  }

  // We support visualization of fake edges only in the head and in the tail of subroute.
  auto constexpr kInvalidId = std::numeric_limits<size_t>::max();
  auto firstReal = kInvalidId;
  auto lastReal = kInvalidId;
  for (size_t i = 0; i < segments.size(); ++i)
  {
    if (!segments[i].GetSegment().IsRealSegment())
      continue;

    if (firstReal == kInvalidId)
      firstReal = i;
    lastReal = i;
  }

  if (firstReal == kInvalidId)
  {
    // All segments are fake.
    subroute->m_headFakeDistance = 0.0;
    subroute->m_tailFakeDistance = 0.0;
  }
  else
  {
    CHECK_NOT_EQUAL(firstReal, kInvalidId, ());
    CHECK_NOT_EQUAL(lastReal, kInvalidId, ());

    auto constexpr kEps = 1e-5;

    // To prevent visual artefacts, in the case when all head segments are real
    // m_headFakeDistance must be less than 0.0.
    auto const headLen = (firstReal > 0) ? segments[firstReal - 1].GetDistFromBeginningMerc() - baseDistance : 0.0;
    if (AlmostEqualAbs(headLen, 0.0, kEps))
      subroute->m_headFakeDistance = -kBias;
    else
      subroute->m_headFakeDistance = headLen;

    // To prevent visual artefacts, in the case when all tail segments are real
    // m_tailFakeDistance must be greater than the length of the subroute.
    auto const subrouteLen = segments.back().GetDistFromBeginningMerc() - baseDistance;
    auto const tailLen = segments[lastReal].GetDistFromBeginningMerc() - baseDistance;
    if (AlmostEqualAbs(tailLen, subrouteLen, kEps))
      subroute->m_tailFakeDistance = subrouteLen + kBias;
    else
      subroute->m_tailFakeDistance = tailLen;
  }

  subroute->m_polyline = m2::PolylineD(std::move(points));
  return subroute;
}
}  // namespace

RoutingManager::RoutingManager(Callbacks && callbacks, Delegate & delegate)
  : m_callbacks(std::move(callbacks))
  , m_sessionRoute(m_routingSession, std::bind_front(&RoutingManager::OnRebuildRouteReady, this),
                   std::bind_front(&RoutingManager::SwapActiveAlternative, this))
  , m_delegate(delegate)
  , m_extrapolator([this](location::GpsInfo const & gpsInfo) { this->OnExtrapolatedLocationUpdate(gpsInfo); })
{
  m_routingSession.Init(
#ifdef SHOW_ROUTE_DEBUG_MARKS
      [this](m2::PointD const & pt)
  {
    if (m_bmManager == nullptr)
      return;
    auto editSession = m_bmManager->GetEditSession();
    editSession.SetIsVisible(UserMark::Type::DEBUG_MARK, true);
    editSession.CreateUserMark<DebugMarkPoint>(pt);
  }
#else
      nullptr
#endif
  );

  m_routingSession.SetRoutingCallbacks([this](RoutesResult const & result, RouterResultCode code)
  { OnBuildRouteReady(result, code); }, [this](RoutesResult const & result, RouterResultCode code)
  { OnRebuildRouteReady(result, code); }, [this](uint64_t routeId, storage::CountriesSet const & absentCountries)
  { OnNeedMoreMaps(routeId, absentCountries); }, [this](RouterResultCode code) { OnRemoveRoute(code); });

  m_routingSession.SetChangeSessionStateCallback([this](SessionState, SessionState state)
  {
    if (state == SessionState::RouteBuilding || state == SessionState::RouteRebuilding)
      ClearAlternativeRoutes();
  });

  m_routingSession.SetCheckpointCallback([this](size_t passedCheckpointIdx)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [this, passedCheckpointIdx]()
    {
      size_t const pointsCount = GetRoutePointsCount();

      // TODO(@bykoianko). Since routing system may invoke callbacks from different threads and here
      // we have to use gui thread, ASSERT is not correct. Uncomment it and delete condition after
      // refactoring of threads usage in routing system.
      // ASSERT_LESS(passedCheckpointIdx, pointsCount, ());
      if (passedCheckpointIdx >= pointsCount)
        return;

      if (passedCheckpointIdx == 0)
        OnRoutePointPassed(RouteMarkType::Start, 0);
      else if (passedCheckpointIdx + 1 == pointsCount)
        OnRoutePointPassed(RouteMarkType::Finish, 0);
      else
        OnRoutePointPassed(RouteMarkType::Intermediate, passedCheckpointIdx - 1);
    });
  });

  m_routingSession.SetSpeedCamShowCallback([this](m2::PointD const & point, double cameraSpeedKmPH)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [this, point, cameraSpeedKmPH]()
    {
      if (m_routeSpeedCamShowCallback)
        m_routeSpeedCamShowCallback(point, cameraSpeedKmPH);

      auto editSession = m_bmManager->GetEditSession();
      auto mark = editSession.CreateUserMark<SpeedCameraMark>(point);

      mark->SetIndex(0);
      if (cameraSpeedKmPH == SpeedCameraOnRoute::kNoSpeedInfo)
        return;

      double speed = cameraSpeedKmPH;
      if (measurement_utils::GetMeasurementUnits() == measurement_utils::Units::Imperial)
        speed = measurement_utils::KmphToMiph(cameraSpeedKmPH);

      mark->SetTitle(strings::to_string(static_cast<int>(speed + 0.5)));
    });
  });

  m_routingSession.SetSpeedCamClearCallback([this]()
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [this]()
    {
      m_bmManager->GetEditSession().ClearGroup(UserMark::Type::SPEED_CAM);
      if (m_routeSpeedCamsClearCallback)
        m_routeSpeedCamsClearCallback();
    });
  });
}

void RoutingManager::SetBookmarkManager(BookmarkManager * bmManager)
{
  m_bmManager = bmManager;
}

void RoutingManager::SetTransitManager(TransitReadManager * transitManager)
{
  m_transitReadManager = transitManager;
}

void RoutingManager::OnBuildRouteReady(RoutesResult const & result, RouterResultCode code)
{
  // @TODO(bykoianko) Remove |code| from callback signature.
  CHECK_EQUAL(code, RouterResultCode::NoError, ());
  HidePreviewSegments();

  auto const hasWarnings = InsertRoute(result);
  m_drapeEngine.SafeCall(&df::DrapeEngine::StopLocationFollow);

  // Validate route (in case of bicycle routing it can be invalid).
  ASSERT(result.IsValid(), ());
  auto const & active = result.GetActive();
  // Do not show the full route if one or more stops were added, for easier multi-stop trip planning.
  if (active.IsValid() && active.GetSubrouteCount() < 2 && m_currentRouterType != routing::RouterType::Ruler)
  {
    m2::RectD routeRect = active.GetLimitRect();
    routeRect.Scale(kRouteScaleMultiplier);
    m_drapeEngine.SafeCall(&df::DrapeEngine::SetModelViewRect, routeRect, true /* applyRotation */, -1 /* zoom */,
                           true /* isAnim */, true /* useVisibleViewport */);
  }

  CallRouteBuilded(hasWarnings ? RouterResultCode::HasWarnings : code, storage::CountriesSet());
}

void RoutingManager::OnRebuildRouteReady(RoutesResult const & result, RouterResultCode code)
{
  HidePreviewSegments();

  if (code != RouterResultCode::NoError)
    return;

  auto const hasWarnings = InsertRoute(result);
  CallRouteBuilded(hasWarnings ? RouterResultCode::HasWarnings : code, storage::CountriesSet());
}

void RoutingManager::OnNeedMoreMaps(uint64_t routeId, storage::CountriesSet const & absentCountries)
{
  // No need to inform user about maps needed for the route if the method is called
  // when RoutingSession contains a new route.
  if (m_routingSession.IsRouteValid() && !m_routingSession.IsRouteId(routeId))
    return;

  HidePreviewSegments();
  CallRouteBuilded(RouterResultCode::NeedMoreMaps, absentCountries);
}

void RoutingManager::OnRemoveRoute(routing::RouterResultCode code)
{
  HidePreviewSegments();
  RemoveRoute(true /* deactivateFollowing */);
  CallRouteBuilded(code, storage::CountriesSet());
}

void RoutingManager::OnRoutePointPassed(RouteMarkType type, size_t intermediateIndex)
{
  // Remove route point.
  ASSERT(m_bmManager != nullptr, ());
  RoutePointsLayout routePoints(*m_bmManager);
  routePoints.PassRoutePoint(type, intermediateIndex);

  if (type == RouteMarkType::Finish)
    RemoveRoute(false /* deactivateFollowing */);

  SaveRoutePoints();
}

void RoutingManager::OnLocationUpdate(location::GpsInfo const & info)
{
  m_extrapolator.OnLocationUpdate(info);
}

RouterType RoutingManager::GetBestRouter(m2::PointD const & startPoint, m2::PointD const & finalPoint) const
{
  // todo Implement something more sophisticated here (or delete the method).
  return GetLastUsedRouter();
}

RouterType RoutingManager::GetLastUsedRouter() const
{
  std::string routerTypeStr;
  if (!settings::Get(kRouterTypeKey, routerTypeStr))
    return RouterType::Vehicle;

  auto const routerType = FromString(routerTypeStr);

  switch (routerType)
  {
  case RouterType::Pedestrian:
  case RouterType::Bicycle:
  case RouterType::Transit:
  case RouterType::Ruler: return routerType;
  default: return RouterType::Vehicle;
  }
}

void RoutingManager::Init(std::shared_ptr<routing::NumMwmIds> ptr)
{
  m_numMwmIDs = std::move(ptr);
  m_numMwmTree = MakeNumMwmTree(*m_numMwmIDs, m_callbacks.m_countryInfoGetter());

  SetRouterImpl(GetLastUsedRouter());
}

void RoutingManager::SetRouterImpl(RouterType type)
{
  // Ruler has no road profile and uses Transit session settings.
  m_loadAltitudes = RoutingOptions::GetVehicleType(type).value_or(VehicleType::Transit) != VehicleType::Car;
  InitSession(m_routingSession, type);
  m_currentRouterType = type;
}

void RoutingManager::InitSession(routing::RoutingSession & session, RouterType type) const
{
  // Ruler has no road profile and uses Transit session settings.
  VehicleType const vehicleType = RoutingOptions::GetVehicleType(type).value_or(VehicleType::Transit);
  bool const loadAltitudes = vehicleType != VehicleType::Car;

  std::unique_ptr<IRouter> router;
  std::unique_ptr<AbsentRegionsFinder> absentFinder;

  if (type == RouterType::Ruler)
    router = std::make_unique<RulerRouter>();
  else
  {
    auto & dataSource = m_callbacks.m_dataSourceGetter();

    auto const countryFileGetter = [this](m2::PointD const & p)
    { return m_callbacks.m_countryInfoGetter().GetRegionCountryId(p); };

    auto const localFileChecker = [this, &dataSource](std::string const & countryFile)
    {
      MwmSet::MwmId mwmId = dataSource.GetMwmIdByCountryFile(platform::CountryFile(countryFile));
      if (!mwmId.IsAlive())
      {
        /// @todo Temporary hack, works only for splitted regions.
        /// Should delegate "old" (outdated) countries check to the Framework.
        platform::CountryFile parent(m_callbacks.m_countryParentNameGetterFn(countryFile));
        if (!parent.IsEmpty())
          mwmId = dataSource.GetMwmIdByCountryFile(parent);
      }

      return mwmId.IsAlive();
    };

    auto const getMwmRectByName = [this](std::string const & countryId)
    { return m_callbacks.m_countryInfoGetter().GetLimitRectForLeaf(countryId); };

    router = std::make_unique<IndexRouter>(vehicleType, loadAltitudes, m_callbacks.m_countryParentNameGetterFn,
                                           countryFileGetter, getMwmRectByName, m_numMwmIDs, m_numMwmTree, session,
                                           dataSource);
    absentFinder = std::make_unique<AbsentRegionsFinder>(countryFileGetter, localFileChecker, m_numMwmIDs, dataSource);
  }

  session.SetRoutingSettings(GetRoutingSettings(vehicleType));
  session.SetRouter(std::move(router), std::move(absentFinder));
}

void RoutingManager::RemoveRoute(bool deactivateFollowing)
{
  GetPlatform().RunTask(Platform::Thread::Gui, [this, deactivateFollowing]()
  {
    {
      auto es = m_bmManager->GetEditSession();
      es.ClearGroup(UserMark::Type::TRANSIT);
      es.ClearGroup(UserMark::Type::SPEED_CAM);
      es.ClearGroup(UserMark::Type::ROAD_WARNING);
      es.ClearGroup(UserMark::Type::ROUTE_ALT);
    }
    if (deactivateFollowing)
      SetPointsFollowingMode(false /* enabled */);
  });

  if (deactivateFollowing)
  {
    m_transitReadManager->BlockTransitSchemeMode(false /* isBlocked */);
    // Remove all subroutes.
    m_drapeEngine.SafeCall(&df::DrapeEngine::RemoveSubroute, dp::DrapeID(), true /* deactivateFollowing */);
  }
  else
  {
    df::DrapeEngineLockGuard lock(m_drapeEngine);
    if (lock)
    {
      std::lock_guard<std::mutex> lockSubroutes(m_drapeSubroutesMutex);
      for (auto const & subrouteId : m_drapeSubroutes)
        lock.Get()->RemoveSubroute(subrouteId, false /* deactivateFollowing */);
    }
  }

  {
    std::lock_guard<std::mutex> lock(m_drapeSubroutesMutex);
    m_drapeSubroutes.clear();
    m_transitRouteInfo = TransitRouteInfo();
  }
}

void RoutingManager::ClearAlternativeRoutes()
{
  // Synchronously clear ETA balloons. RemoveRoute uses RunTask(Gui) which only fires after
  // the current GUI flow returns, leaving stale marks briefly visible; we call the same path
  // directly since RoutingManager is GUI-thread-only.
  m_bmManager->GetEditSession().ClearGroup(UserMark::Type::ROUTE_ALT);

  m_drapeEngine.SafeCall(&df::DrapeEngine::RemoveAlternativeSubroutes);
}

void RoutingManager::CollectRoadWarnings(std::vector<routing::RouteSegment> const & segments,
                                         m2::PointD const & startPt, double baseDistance,
                                         RoadWarningsCollection & roadWarnings)
{
  double currentDistance = baseDistance;
  double startDistance = baseDistance;
  RoadWarningMarkType lastWarn = RoadWarningMarkType::Count;
  for (size_t i = 0; i < segments.size(); ++i)
  {
    auto const currentWarn = ChooseRoadWarning(segments[i].GetRoadTypes(), m_currentRouterType);
    if (currentWarn != lastWarn)
    {
      if (lastWarn != RoadWarningMarkType::Count)
      {
        ASSERT(!roadWarnings[lastWarn].empty(), ());
        roadWarnings[lastWarn].back().m_distance = segments[i].GetDistFromBeginningMeters() - startDistance;
      }

      if (currentWarn != RoadWarningMarkType::Count)
      {
        startDistance = currentDistance;
        auto const featureId =
            FeatureID(GetMwmId(segments[i].GetSegment().GetMwmId()), segments[i].GetSegment().GetFeatureId());
        auto const markPoint = i == 0 ? startPt : segments[i - 1].GetJunction().GetPoint();
        roadWarnings[currentWarn].push_back(RoadInfo(markPoint, featureId));
      }
      lastWarn = currentWarn;
    }
    currentDistance = segments[i].GetDistFromBeginningMeters();
  }
  if (lastWarn != RoadWarningMarkType::Count)
    roadWarnings[lastWarn].back().m_distance = segments.back().GetDistFromBeginningMeters() - startDistance;
}

void RoutingManager::CollectRoadPointWarnings(RouteBase const & route, RoadWarningsCollection & roadWarnings)
{
  // The heavy barrier lookup already ran on the routing thread (IndexRouter::RedressRoute);
  // here we just translate the stored barrier types into UI mark types and filter by router type.
  auto const & checker = BarrierWarningChecker::Instance();
  for (auto const & warning : route.GetWarnings())
  {
    auto const markType = checker.GetWarningType(warning.m_type);
    if (markType == RoadWarningMarkType::Count || !IsWarningShownFor(markType, m_currentRouterType))
      continue;

    // Check for duplicates (from alt routes).
    RoadInfo const toInsert(warning.m_point, warning.m_featureId);
    auto & resVec = roadWarnings[markType];
    if (!base::IsExistIf(resVec, [&toInsert](RoadInfo const & ri)
    {
      return ri.m_featureId == toInsert.m_featureId &&
             ri.m_startPoint.EqualDxDy(toInsert.m_startPoint, kMwmPointAccuracy);
    }))
      resVec.push_back(toInsert);
  }
}

void RoutingManager::CreateRoadWarningMarks(RoadWarningsCollection && roadWarnings)
{
  if (roadWarnings.empty())
    return;

  GetPlatform().RunTask(Platform::Thread::Gui, [this, roadWarnings = std::move(roadWarnings)]()
  {
    auto es = m_bmManager->GetEditSession();
    for (auto const & typeInfo : roadWarnings)
    {
      auto const type = typeInfo.first;
      for (size_t i = 0; i < typeInfo.second.size(); ++i)
      {
        auto const & routeInfo = typeInfo.second[i];
        auto mark = es.CreateUserMark<RoadWarningMark>(routeInfo.m_startPoint);
        mark->SetIndex(static_cast<uint32_t>(i));
        mark->SetRoadWarningType(type);
        mark->SetFeatureId(routeInfo.m_featureId);
        // Point warnings (gate/lift_gate) sit on a single vertex and carry no span length.
        if (routeInfo.m_distance > 0.0)
          mark->SetDistance(platform::Distance::CreateFormatted(routeInfo.m_distance).ToString());
      }
    }
  });
}

namespace
{
// Multiplier applied to the alpha channel of subroute colors for alternative (non-active) routes.
float constexpr kAlternativeRouteAlphaMul = 0.5f;

}  // namespace

void RoutingManager::CreateRouteAltMarks(routing::RoutesResult const & result)
{
  if (result.m_routes.empty())
    return;

  // Snapshot the data we need so the Gui-thread task doesn't depend on |result|'s lifetime.
  struct AltMarkInfo
  {
    m2::PointD m_pt;
    std::string m_eta;
    size_t m_idx;
    bool m_isActive;
  };
  std::vector<AltMarkInfo> infos;
  infos.reserve(result.m_routes.size());

  for (size_t i = 0; i < result.m_routes.size(); ++i)
  {
    auto const & r = result.m_routes[i];
    if (!r.IsValid())
      continue;

    // Alts carry a divergence midpoint (set by IndexRouter::CalculateRoute) so the balloon lands
    // where the alt actually differs from the active route. Active route has no diff — fall back
    // to the geometric midpoint of the whole route.
    auto const & diffMid = r.GetDiffMidpoint();
    m2::PointD const pivot = diffMid ? *diffMid : r.GetMidpoint();
    infos.push_back({pivot, platform::Duration(std::lround(r.GetTotalTimeSec())).GetHoursMinutesString(), i,
                     i == result.m_activeIdx});
  }

  GetPlatform().RunTask(Platform::Thread::Gui, [this, routesId = result.m_routesId, infos = std::move(infos)]()
  {
    // A rebuild or navigation can hide choices before this queued task runs.
    bool hasAlternatives = false;
    m_routingSession.RouteCall([&](RoutesResult const & current)
    { hasAlternatives = current.m_routesId == routesId && current.m_routes.size() > 1; });
    if (!hasAlternatives || m_routingSession.IsFollowing())
      return;

    // Place each balloon up or down based on the midpoint's latitude relative to the others:
    // the northern midpoint (larger mercator y) gets the up balloon, the southern one goes down.
    // +y in drape vertex-normal space is downward, so (0, -N) lifts the body above the pivot.
    float constexpr kAltMarkOffsetPx = 50.0f;
    double avgY = 0.0;
    for (auto const & info : infos)
      avgY += info.m_pt.y;
    avgY /= static_cast<double>(infos.size());

    auto es = m_bmManager->GetEditSession();
    for (auto const & info : infos)
    {
      auto mark = es.CreateUserMark<RouteAltMark>(info.m_pt);
      mark->SetEta(info.m_eta);
      mark->SetRouteIdx(info.m_idx);
      mark->SetIsActive(info.m_isActive);
      float const sign = (info.m_pt.y >= avgY) ? -1.0f : 1.0f;
      mark->SetPixelOffset({0.0f, sign * kAltMarkOffsetPx});
    }
  });
}

MwmSet::MwmId RoutingManager::GetMwmId(routing::NumMwmId numMwmId) const
{
  return m_callbacks.m_dataSourceGetter().GetMwmIdByCountryFile(m_numMwmIDs->GetFile(numMwmId));
}

bool RoutingManager::InsertRoute(RoutesResult const & result)
{
  if (!m_drapeEngine || result.m_routes.empty())
    return false;

  // TODO: Now we always update whole route, so we need to remove previous one.
  RemoveRoute(false /* deactivateFollowing */);

  RoadWarningsCollection roadWarnings;

  bool const isTransitRoute = (m_currentRouterType == RouterType::Transit);
  auto const makeTransitRouteDisplay = [this]()
  {
    // clang-format off
    return std::make_shared<TransitRouteDisplay>(*m_transitReadManager,
          [this](routing::NumMwmId numMwmId) { return GetMwmId(numMwmId); },
          m_callbacks.m_stringsBundleGetter, m_bmManager, m_transitSymbolSizes);
    // clang-format on
  };

  std::shared_ptr<TransitRouteDisplay> transitRouteDisplay;
  if (isTransitRoute)
    transitRouteDisplay = makeTransitRouteDisplay();

  // Alternatives are drawn in follow (navigation) mode too: the car may turn to one of them, and it is
  // followed then without a rebuild. ETA balloons would clutter the moving map, the ETA is shown in the
  // navigation UI instead.
  bool const isFollowing = m_routingSession.IsFollowing();
  for (size_t i = 0; i < result.m_routes.size(); ++i)
  {
    if (i == result.m_activeIdx)
      continue;
    // A TransitRouteDisplay accumulates steps/distance across all subroutes fed to it, so an
    // alternative route must use its own throwaway display: it draws just its (muted) polyline,
    // without corrupting the active route's distance/steps or duplicating its stop marks (the
    // alt's display is never asked for route info or marks).
    auto const altDisplay = isTransitRoute ? makeTransitRouteDisplay() : transitRouteDisplay;
    InsertSingleRoute(result.m_routes[i], false /* isActive */,
                      result.m_routes[i].GetSharedStartMerc(result.GetActive()), 0.0 /* depthOffset */, altDisplay,
                      roadWarnings);
  }
  // Lift the active route by 10 so it stays above alternative subroutes even when polylines overlap.
  // The offset must exceed the per-route subroute count (count is typically 1, so 10 is plenty).
  InsertSingleRoute(result.GetActive(), true /* isActive */, 0.0 /* sharedWithActiveMerc */, 10.0 /* depthOffset */,
                    transitRouteDisplay, roadWarnings);

  if (!isFollowing && m_currentRouterType != RouterType::Ruler && result.m_routes.size() >= 2)
    CreateRouteAltMarks(result);

  {
    std::lock_guard<std::mutex> lock(m_drapeSubroutesMutex);
    m_transitRouteInfo = isTransitRoute ? transitRouteDisplay->GetRouteInfo() : TransitRouteInfo();
  }

  if (isTransitRoute)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [transitRouteDisplay = std::move(transitRouteDisplay)]()
    { transitRouteDisplay->CreateTransitMarks(); });
  }

  // HasWarnings signals avoidable car-road warnings. Walking and cycling still render their warning
  // marks and expose ferry avoidance directly.
  // Steps/gate/lift_gate have no avoidance control.
  bool const hasDrivingOptionsWarning =
      m_currentRouterType == RouterType::Vehicle &&
      base::AnyOf(roadWarnings, [](auto const & w) { return IsAvoidableRoadWarning(w.first); });

  if (!roadWarnings.empty())
    CreateRoadWarningMarks(std::move(roadWarnings));

  return hasDrivingOptionsWarning;
}

void RoutingManager::InsertSingleRoute(RouteBase const & route, bool isActive, double sharedWithActiveMerc,
                                       double depthOffset,
                                       std::shared_ptr<TransitRouteDisplay> const & transitRouteDisplay,
                                       RoadWarningsCollection & roadWarnings)
{
  if (!route.IsValid())
    return;

  float const alphaMul = isActive ? 1.0f : kAlternativeRouteAlphaMul;

  std::vector<RouteSegment> segments;
  double distance = 0.0;
  auto const subroutesCount = route.GetSubrouteCount();
  for (size_t subrouteIndex = route.GetCurrentSubrouteIdx(); subrouteIndex < subroutesCount; ++subrouteIndex)
  {
    route.GetSubrouteInfo(subrouteIndex, segments);

    auto const startPt = route.GetSubrouteAttrs(subrouteIndex).GetStart().GetPoint();
    auto subroute =
        CreateDrapeSubroute(segments, startPt, distance,
                            static_cast<double>(subroutesCount - subrouteIndex - 1) + depthOffset, m_currentRouterType);
    if (!subroute)
      continue;
    subroute->m_alphaMul = alphaMul;
    subroute->m_sharedWithActiveDistance = sharedWithActiveMerc;
    distance = segments.back().GetDistFromBeginningMerc();
    switch (m_currentRouterType)
    {
    case RouterType::Vehicle:
    {
      subroute->m_routeType = df::RouteType::Car;
      subroute->AddStyle(df::SubrouteStyle(df::kRouteColor, df::kRouteOutlineColor));
      // Skip traffic colors on alternatives — keep them visually muted and easy to distinguish.
      if (isActive)
      {
        FillTrafficForRendering(segments, subroute->m_traffic);
        FillTurnsDistancesForRendering(segments, subroute->m_baseDistance, subroute->m_turns);
      }
      break;
    }
    case RouterType::Transit:
    {
      subroute->m_routeType = df::RouteType::Transit;
      if (!transitRouteDisplay->ProcessSubroute(segments, *subroute.get()))
        continue;
      break;
    }
    case RouterType::Pedestrian:
    {
      subroute->m_routeType = df::RouteType::Pedestrian;
      subroute->AddStyle(df::SubrouteStyle(df::kRoutePedestrian, df::RoutePattern(4.0, 2.0)));
      break;
    }
    case RouterType::Bicycle:
    {
      subroute->m_routeType = df::RouteType::Bicycle;
      subroute->AddStyle(df::SubrouteStyle(df::kRouteBicycle, df::RoutePattern(8.0, 2.0)));
      if (isActive)
        FillTurnsDistancesForRendering(segments, subroute->m_baseDistance, subroute->m_turns);
      break;
    }
    case RouterType::Ruler:
    {
      subroute->m_routeType = df::RouteType::Ruler;
      subroute->AddStyle(df::SubrouteStyle(df::kRouteRuler, df::RoutePattern(16.0, 2.0)));
      break;
    }
    default: CHECK(false, ("Unknown router type"));
    }

    CollectRoadWarnings(segments, startPt, subroute->m_baseDistance, roadWarnings);

    auto const subrouteId =
        m_drapeEngine.SafeCallWithResult(&df::DrapeEngine::AddSubroute, df::SubrouteConstPtr(subroute.release()));

    std::lock_guard<std::mutex> lock(m_drapeSubroutesMutex);
    m_drapeSubroutes.push_back(subrouteId);
  }

  // Point warnings (barrier nodes) are precomputed on the routing thread (IndexRouter::RedressRoute)
  // and stored in the route; read them once (route-global, not per-subroute).
  CollectRoadPointWarnings(route, roadWarnings);
}

void RoutingManager::FollowRoute()
{
  if (!m_routingSession.EnableFollowMode())
    return;

  m_transitReadManager->BlockTransitSchemeMode(true /* isBlocked */);

  // Switching on the extrapolator only for following mode in car and bicycle navigation.
  m_extrapolator.Enable(m_currentRouterType == RouterType::Vehicle || m_currentRouterType == RouterType::Bicycle);
  m_delegate.OnRouteFollow(m_currentRouterType);

  m_bmManager->GetEditSession().ClearGroup(UserMark::Type::ROAD_WARNING);
  HideRoutePoint(RouteMarkType::Start);
  SetPointsFollowingMode(true /* enabled */);

  // The alternatives stay on the map while navigating, only their ETA balloons are removed.
  m_bmManager->GetEditSession().ClearGroup(UserMark::Type::ROUTE_ALT);

  CancelRecommendation(Recommendation::RebuildAfterPointsLoading);
}

bool RoutingManager::SwapActiveAlternative(size_t idx)
{
  if (!m_routingSession.SwapActiveAlternative(idx))
    return false;

  // Re-render drape with the new active variant and notify platform UI (elevation profile,
  // route info, etc.) via the same RouteBuilded callback path used by the initial build, so
  // any cached route data on the Android/iOS side is refreshed for the new active route.
  bool hasWarnings = false;
  m_routingSession.RouteCall([this, &hasWarnings](routing::RoutesResult const & result)
  { hasWarnings = InsertRoute(result); });
  CallRouteBuilded(hasWarnings ? RouterResultCode::HasWarnings : RouterResultCode::NoError, storage::CountriesSet());
  return true;
}

namespace
{
// A negative bearing is unknown, NaN can't be used with -ffast-math.
double constexpr kAnyBearing = -1.0;

double FindRouteAnchor(std::vector<m2::PointD> const & points, m2::PointD const & point, double bearingDeg,
                       size_t & anchorIdx, m2::PointD & anchor);
}  // namespace

SessionRoute::SessionRoute(routing::RoutingSession & session, routing::ReadyCallback && onRebuilt,
                           SwapAlternativeFn && swapAlternative)
  : m_session(session)
  , m_onRebuilt(std::move(onRebuilt))
  , m_swapAlternative(std::move(swapAlternative))
{}

bool SessionRoute::SwapToAlternativeUnderCar(location::GpsInfo const & info)
{
  // The car is on the road of an alternative, not on a road beside it.
  double constexpr kOnAlternativeM = 20.0;
  if (!m_session.IsFollowing() || !m_session.IsRouteValid())
    return false;

  m2::PointD const position = mercator::FromLatLon(info.m_latitude, info.m_longitude);
  double const bearingDeg = info.HasBearing() ? info.m_bearing : kAnyBearing;
  int targetIdx = -1;
  double bestDistanceM = kOnAlternativeM;
  m_session.RouteCall([&](routing::RoutesResult const & result)
  {
    for (size_t i = 0; i < result.m_routes.size(); ++i)
    {
      if (i == result.m_activeIdx)
        continue;
      std::vector<m2::PointD> points;
      result.m_routes[i].ForEachPoint([&points](geometry::PointWithAltitude const & p)
      { points.push_back(p.GetPoint()); });
      if (points.size() < 2)
        continue;
      size_t anchorIdx = 0;
      m2::PointD anchor;
      // Only a part going the way the car goes: the alternative may pass the car the other way.
      double const distanceM = FindRouteAnchor(points, position, bearingDeg, anchorIdx, anchor);
      if (distanceM < bestDistanceM)
      {
        bestDistanceM = distanceM;
        targetIdx = static_cast<int>(i);
      }
    }
  });
  if (targetIdx < 0)
    return false;

  LOG(LINFO, ("The car is on the alternative route", targetIdx, "at", bestDistanceM, "m, following it"));
  return m_swapAlternative(static_cast<size_t>(targetIdx));
}

bool SessionRoute::IsLeavingRoute(SessionState state) const
{
  return state == SessionState::RouteNeedRebuild ||
         ((state == SessionState::OnRoute || state == SessionState::RouteNotStarted) &&
          m_session.IsMovingAwayFromRoute());
}

bool RoutingManager::TryTapOnAlternativeRoute(m2::PointD const & mercator, double mercatorPerPixel)
{
  // Alts aren't drawn during navigation and the active route shouldn't be tap-swappable.
  if (!IsRoutingActive() || m_routingSession.IsFollowing() || !m_routingSession.IsRouteValid())
    return false;

  // Pixel-radius for the tap area. Matches the visual half-width the routes are drawn with;
  // closer than this and we treat the tap as hitting that polyline.
  double constexpr kTapPixels = 16.0;
  double const tapMerc = kTapPixels * df::VisualParams::Instance().GetVisualScale() * mercatorPerPixel;
  double const tapMercSq = tapMerc * tapMerc;
  m2::RectD const tapRect(mercator, tapMerc, tapMerc);

  int targetIdx = -1;
  double bestSq = tapMercSq;
  m_routingSession.RouteCall([&](routing::RoutesResult const & result)
  {
    for (size_t i = 0; i < result.m_routes.size(); ++i)
    {
      if (i == result.m_activeIdx)
        continue;

      std::optional<m2::PointD> prev;
      result.m_routes[i].ForEachPoint([&](geometry::PointWithAltitude const & p)
      {
        if (prev)
        {
          auto const & p2 = p.GetPoint();
          if (m2::RectD(*prev, p2).IsIntersect(tapRect))
          {
            m2::ParametrizedSegment<m2::PointD> seg(*prev, p2);
            double const distSq = seg.SquaredDistanceToPoint(mercator);
            if (distSq < bestSq)
            {
              bestSq = distSq;
              targetIdx = i;
            }
          }
        }
        prev = p.GetPoint();
      });
    }
  });

  return targetIdx >= 0 ? SwapActiveAlternative(targetIdx) : false;
}

void RoutingManager::CloseRouting(bool removeRoutePoints)
{
  m_extrapolator.Enable(false);
  // Hide preview.
  HidePreviewSegments();

  if (m_routingSession.IsBuilt())
    m_routingSession.EmitCloseRoutingEvent();
  m_routingSession.Reset();
  RemoveRoute(true /* deactivateFollowing */);

  if (removeRoutePoints)
  {
    m_bmManager->GetEditSession().ClearGroup(UserMark::Type::ROUTING);
    CancelRecommendation(Recommendation::RebuildAfterPointsLoading);
  }
}

void RoutingManager::SetLastUsedRouter(RouterType type)
{
  settings::Set(kRouterTypeKey, ToString(type));
}

void RoutingManager::HideRoutePoint(RouteMarkType type, size_t intermediateIndex)
{
  RoutePointsLayout routePoints(*m_bmManager);
  RouteMarkPoint * mark = routePoints.GetRoutePointForEdit(type, intermediateIndex);
  if (mark != nullptr)
    mark->SetIsVisible(false);
}

bool RoutingManager::IsMyPosition(RouteMarkType type, size_t intermediateIndex)
{
  RoutePointsLayout routePoints(*m_bmManager);
  RouteMarkPoint const * mark = routePoints.GetRoutePoint(type, intermediateIndex);
  return mark != nullptr && mark->IsMyPosition();
}

std::vector<RouteMarkData> RoutingManager::GetRoutePoints() const
{
  std::vector<RouteMarkData> result;
  RoutePointsLayout routePoints(*m_bmManager);
  for (auto const & p : routePoints.GetRoutePoints())
    result.push_back(p->GetMarkData());
  return result;
}

size_t RoutingManager::GetRoutePointsCount() const
{
  RoutePointsLayout routePoints(*m_bmManager);
  return routePoints.GetRoutePointsCount();
}

bool RoutingManager::CouldAddIntermediatePoint() const
{
  if (!IsRoutingActive())
    return false;

  return m_bmManager->GetUserMarkIds(UserMark::Type::ROUTING).size() < RoutePointsLayout::kMaxRoutePointsCount;
}

bool RoutingManager::AddRoutePoint(RouteMarkData && markData, bool optimize)
{
  ASSERT(m_bmManager != nullptr, ());
  RoutePointsLayout routePoints(*m_bmManager);
  bool const isIntermediate = markData.m_pointType == RouteMarkType::Intermediate;

  if (!isIntermediate)
    routePoints.RemoveRoutePoint(markData.m_pointType);

  if (markData.m_isMyPosition)
  {
    if (auto const * staleMyPosition = routePoints.GetMyPositionPoint())
      routePoints.RemoveRoutePoint(staleMyPosition->GetRoutePointType(), staleMyPosition->GetIntermediateIndex());
  }

  bool const hasStart = routePoints.GetRoutePoint(RouteMarkType::Start) != nullptr;
  bool const hasFinish = routePoints.GetRoutePoint(RouteMarkType::Finish) != nullptr;
  if (isIntermediate)
    markData.m_intermediateIndex = routePoints.GetRoutePointsCount() - hasStart - hasFinish;

  markData.m_isVisible = !markData.m_isMyPosition;
  auto const * addedPoint = routePoints.AddRoutePoint(std::move(markData));
  if (addedPoint == nullptr)
    return false;

  // Ruler points describe the user's measurement, not a route to optimize.
  if (isIntermediate && optimize && hasStart && hasFinish && m_currentRouterType != RouterType::Ruler)
    ReorderIntermediatePoints(routePoints, addedPoint);
  return true;
}

void RoutingManager::ReplaceRoutePoint(RouteMarkType type, size_t intermediateIndex, RouteMarkData && markData)
{
  ASSERT(m_bmManager != nullptr, ());
  RoutePointsLayout routePoints(*m_bmManager);
  auto * target = routePoints.GetRoutePointForEdit(type, intermediateIndex);
  if (target == nullptr)
  {
    markData.m_pointType = type;
    AddRoutePoint(std::move(markData), false /* optimize */);
    return;
  }

  if (markData.m_isMyPosition)
  {
    if (auto const * staleMyPosition = routePoints.GetMyPositionPoint();
        staleMyPosition != nullptr && staleMyPosition != target)
    {
      routePoints.RemoveRoutePoint(staleMyPosition->GetRoutePointType(), staleMyPosition->GetIntermediateIndex());
    }
  }

  // Removing another My Position mark may have changed the target's type or index.
  markData.m_pointType = target->GetRoutePointType();
  markData.m_intermediateIndex = target->GetIntermediateIndex();
  markData.m_isVisible = !markData.m_isMyPosition;
  target->SetMarkData(std::move(markData));
}

bool RoutingManager::OptimizeRoutePoints()
{
  ASSERT(m_bmManager != nullptr, ());
  if (IsRoutingFollowing() || m_currentRouterType == RouterType::Ruler ||
      m_bmManager->GetUserMarkIds(UserMark::Type::ROUTING).size() < 3)
    return false;
  RoutePointsLayout layout(*m_bmManager);
  if (layout.GetRoutePoint(RouteMarkType::Start) == nullptr || layout.GetRoutePoint(RouteMarkType::Finish) == nullptr)
    return false;
  return ReorderIntermediatePoints(layout, nullptr /* added */);
}

bool RoutingManager::ContinueRouteToPoint(RouteMarkData && markData)
{
  ASSERT(m_bmManager != nullptr, ());
  ASSERT(markData.m_pointType == RouteMarkType::Finish, ("New route point should have type RouteMarkType::Finish"));
  RoutePointsLayout routePoints(*m_bmManager);

  if (routePoints.GetRoutePointsCount() >= RoutePointsLayout::kMaxRoutePointsCount)
  {
    LOG(LWARNING, ("Cannot continue route: route points limit reached."));
    return false;
  }

  // Finish point is now Intermediate point
  RouteMarkPoint * finishMarkData = routePoints.GetRoutePointForEdit(RouteMarkType::Finish);
  if (finishMarkData == nullptr)
  {
    LOG(LWARNING, ("Cannot continue route: finish point is missing."));
    return false;
  }
  finishMarkData->SetRoutePointType(RouteMarkType::Intermediate);
  finishMarkData->SetIntermediateIndex(routePoints.GetRoutePointsCount() - 2);

  if (markData.m_isMyPosition)
  {
    RouteMarkPoint const * mark = routePoints.GetMyPositionPoint();
    if (mark)
      routePoints.RemoveRoutePoint(mark->GetRoutePointType(), mark->GetIntermediateIndex());
  }

  markData.m_intermediateIndex = routePoints.GetRoutePointsCount() - 1;
  markData.m_isVisible = !markData.m_isMyPosition;
  routePoints.AddRoutePoint(std::move(markData));
  return true;
}

void RoutingManager::RemoveRoutePoint(RouteMarkType type, size_t intermediateIndex)
{
  ASSERT(m_bmManager != nullptr, ());
  RoutePointsLayout routePoints(*m_bmManager);
  routePoints.RemoveRoutePoint(type, intermediateIndex);
}

void RoutingManager::RemoveRoutePoints()
{
  ASSERT(m_bmManager != nullptr, ());
  RoutePointsLayout routePoints(*m_bmManager);
  routePoints.RemoveRoutePoints();
}

void RoutingManager::RemoveIntermediateRoutePoints()
{
  ASSERT(m_bmManager != nullptr, ());
  RoutePointsLayout routePoints(*m_bmManager);
  routePoints.RemoveIntermediateRoutePoints();
}

void RoutingManager::RemovePassedRoutePoints()
{
  ASSERT(m_bmManager != nullptr, ());
  RoutePointsLayout routePoints(*m_bmManager);
  if (!routePoints.RemovePassedRoutePoints())
    return;

  // If the passed Start was removed, add a new one at the current position.
  if (routePoints.GetRoutePoint(RouteMarkType::Start) == nullptr)
  {
    RouteMarkData startPt;
    startPt.m_pointType = RouteMarkType::Start;
    startPt.m_isMyPosition = true;
    startPt.m_isVisible = false;
    startPt.m_position = m_bmManager->MyPositionMark().GetPivot();
    routePoints.AddRoutePoint(std::move(startPt));
  }
}

void RoutingManager::MoveRoutePoint(RouteMarkType currentType, size_t currentIntermediateIndex,
                                    RouteMarkType targetType, size_t targetIntermediateIndex)
{
  ASSERT(m_bmManager != nullptr, ());
  RoutePointsLayout routePoints(*m_bmManager);
  routePoints.MoveRoutePoint(currentType, currentIntermediateIndex, targetType, targetIntermediateIndex);
}

void RoutingManager::MoveRoutePoint(size_t currentIndex, size_t targetIndex)
{
  ASSERT(m_bmManager != nullptr, ());

  RoutePointsLayout routePoints(*m_bmManager);
  size_t const sz = routePoints.GetRoutePointsCount();
  if (sz == 1)
  {
    auto * point = routePoints.GetRoutePoints().front();
    auto const currentType = point->GetRoutePointType();
    ASSERT(currentType != RouteMarkType::Intermediate, ());
    ASSERT_LESS(currentIndex, 2, ());
    ASSERT_LESS(targetIndex, 2, ());
    ASSERT_NOT_EQUAL(currentIndex, targetIndex, ());
    // Either endpoint slot may be dragged, so flip the real point regardless of drag direction.
    point->SetRoutePointFullType(currentType == RouteMarkType::Start ? RouteMarkType::Finish : RouteMarkType::Start, 0);
    return;
  }
  auto const convertIndex = [sz](RouteMarkType & type, size_t & index)
  {
    if (index == 0)
    {
      type = RouteMarkType::Start;
      index = 0;
    }
    else if (index + 1 == sz)
    {
      type = RouteMarkType::Finish;
      index = 0;
    }
    else
    {
      type = RouteMarkType::Intermediate;
      --index;
    }
  };
  RouteMarkType currentType;
  RouteMarkType targetType;

  convertIndex(currentType, currentIndex);
  convertIndex(targetType, targetIndex);

  routePoints.MoveRoutePoint(currentType, currentIndex, targetType, targetIndex);
}

bool RoutingManager::ReverseRoutePoints()
{
  ASSERT(m_bmManager != nullptr, ());
  RoutePointsLayout routePoints(*m_bmManager);
  auto const points = routePoints.GetRoutePoints();
  if (points.size() < 2 || points.front()->GetRoutePointType() != RouteMarkType::Start ||
      points.back()->GetRoutePointType() != RouteMarkType::Finish)
    return false;

  for (size_t i = 0; i < points.size(); ++i)
  {
    size_t const reversedIndex = points.size() - 1 - i;
    auto * point = points[i];
    if (reversedIndex == 0)
      point->SetRoutePointFullType(RouteMarkType::Start, 0);
    else if (reversedIndex + 1 == points.size())
      point->SetRoutePointFullType(RouteMarkType::Finish, 0);
    else
      point->SetRoutePointFullType(RouteMarkType::Intermediate, reversedIndex - 1);

    point->SetPassed(false);
    point->SetIsVisible(!point->IsMyPosition());
  }
  return true;
}

void RoutingManager::SetPointsFollowingMode(bool enabled)
{
  ASSERT(m_bmManager != nullptr, ());
  RoutePointsLayout routePoints(*m_bmManager);
  routePoints.SetFollowingMode(enabled);
}

bool RoutingManager::ReorderIntermediatePoints(RoutePointsLayout & layout, RouteMarkPoint const * added)
{
  auto const points = layout.GetRoutePoints();
  ASSERT_GREATER_OR_EQUAL(points.size(), 3, ());
  ASSERT(points.front()->GetRoutePointType() == RouteMarkType::Start, ());
  ASSERT(points.back()->GetRoutePointType() == RouteMarkType::Finish, ());

  // Passed stops keep their slots; the rest of the route starts from the last of them.
  size_t lastPassedIndex = 0;
  for (size_t i = 1; i + 1 < points.size(); ++i)
    if (points[i]->IsPassed())
      lastPassedIndex = i;

  std::vector<RouteMarkPoint *> ordered;
  std::vector<RouteMarkPoint *> toPlace;
  for (size_t i = lastPassedIndex + 1; i + 1 < points.size(); ++i)
    if (added == nullptr || points[i] == added)
      toPlace.push_back(points[i]);
    else
      ordered.push_back(points[i]);

  std::vector<m2::PointD> positions;
  for (auto const * point : ordered)
    positions.push_back(point->GetPivot());

  CheckpointPredictor const predictor(points[lastPassedIndex]->GetPivot(), points.back()->GetPivot());
  for (auto * point : toPlace)
  {
    size_t const index = predictor.PredictPosition(positions, point->GetPivot());
    positions.insert(positions.begin() + index, point->GetPivot());
    ordered.insert(ordered.begin() + index, point);
  }

  // Greedy re-insertion depends on the current order and may produce a longer route. Commit only a strictly shorter
  // one to prevent repeated optimization from flipping between orders.
  if (added == nullptr)
  {
    double currentLength = 0.0;
    for (size_t i = lastPassedIndex; i + 1 < points.size(); ++i)
      currentLength += mercator::DistanceOnEarth(points[i]->GetPivot(), points[i + 1]->GetPivot());

    double optimizedLength = 0.0;
    m2::PointD const * previous = &points[lastPassedIndex]->GetPivot();
    for (auto const & position : positions)
    {
      optimizedLength += mercator::DistanceOnEarth(*previous, position);
      previous = &position;
    }
    optimizedLength += mercator::DistanceOnEarth(*previous, points.back()->GetPivot());
    if (optimizedLength >= currentLength)
      return false;
  }

  bool changed = false;
  for (size_t i = 0; i < ordered.size(); ++i)
  {
    size_t const index = lastPassedIndex + i;
    if (ordered[i]->GetIntermediateIndex() != index)
    {
      ordered[i]->SetIntermediateIndex(index);
      changed = true;
    }
  }
  return changed;
}

void RoutingManager::GenerateNotifications(std::vector<std::string> & turnNotifications, bool announceStreets)
{
  m_routingSession.GenerateNotifications(turnNotifications, announceStreets);
}

void RoutingManager::BuildRoute(uint32_t timeoutSec)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ("BuildRoute"));

  m_bmManager->GetEditSession().ClearGroup(UserMark::Type::TRANSIT);

  // Remove already passed intermediate points so they don't affect the new route.
  // https://github.com/organicmaps/organicmaps/issues/7939
  // https://github.com/organicmaps/organicmaps/issues/9592
  // https://github.com/organicmaps/organicmaps/issues/11256
  RemovePassedRoutePoints();

  auto routePoints = GetRoutePoints();
  if (routePoints.size() < 2)
  {
    CallRouteBuilded(RouterResultCode::Cancelled, storage::CountriesSet());
    CloseRouting(false /* remove route points */);
    return;
  }

  // Update my position.
  for (auto & p : routePoints)
  {
    if (!p.m_isMyPosition)
      continue;

    auto const & myPosition = m_bmManager->MyPositionMark();
    if (!myPosition.HasPosition())
    {
      CallRouteBuilded(RouterResultCode::NoCurrentPosition, storage::CountriesSet());
      return;
    }
    p.m_position = myPosition.GetPivot();
  }

  // Check for equal points.
  for (size_t i = 0; i < routePoints.size(); i++)
  {
    for (size_t j = i + 1; j < routePoints.size(); j++)
    {
      if (routePoints[i].m_position.EqualDxDy(routePoints[j].m_position, mercator::kPointEqualityEps))
      {
        CallRouteBuilded(RouterResultCode::Cancelled, storage::CountriesSet());
        CloseRouting(false /* remove route points */);
        return;
      }
    }
  }

  if (IsRoutingActive())
    CloseRouting(false /* remove route points */);

  ShowPreviewSegments(routePoints);

  // Route points preview.
  // Disabled preview zoom to fix https://github.com/organicmaps/organicmaps/issues/5409.
  // Uncomment next lines to enable back zoom on route point add/remove.

  // m2::RectD rect = ShowPreviewSegments(routePoints);
  // rect.Scale(kRouteScaleMultiplier);
  // m_drapeEngine.SafeCall(&df::DrapeEngine::SetModelViewRect, rect, true /* applyRotation */,
  //                        -1 /* zoom */, true /* isAnim */, true /* useVisibleViewport */);

  m_routingSession.ClearPositionAccumulator();
  m_routingSession.SetUserCurrentPosition(routePoints.front().m_position);

  std::vector<m2::PointD> points;
  points.reserve(routePoints.size());
  for (auto const & point : routePoints)
    points.push_back(point.m_position);

  m_routingSession.BuildRoute(Checkpoints(std::move(points)), timeoutSec);
}

void RoutingManager::SetUserCurrentPosition(m2::PointD const & position)
{
  m_routingSession.PushPositionAccumulator(position);

  if (IsRoutingActive())
    m_routingSession.SetUserCurrentPosition(position);

  if (m_routeRecommendCallback != nullptr)
  {
    // Check if we've found my position almost immediately after route points loading.
    auto constexpr kFoundLocationInterval = 2.0;
    auto const elapsed = std::chrono::steady_clock::now() - m_loadRoutePointsTimestamp;
    auto const sec = std::chrono::duration_cast<std::chrono::duration<double>>(elapsed).count();
    if (sec <= kFoundLocationInterval)
    {
      m_routeRecommendCallback(Recommendation::RebuildAfterPointsLoading);
      CancelRecommendation(Recommendation::RebuildAfterPointsLoading);
    }
  }
}

static std::string GetNameFromPoint(RouteMarkData const & rmd)
{
  if (rmd.m_subTitle.empty())
    return "";
  return rmd.m_title;
}

kml::TrackId RoutingManager::SaveRoute()
{
  RouteJunctions junctions;
  if (!RoutingSession().GetRouteJunctionPoints(junctions))
    return kml::kInvalidTrackId;

  base::Unique(junctions, [](geometry::PointWithAltitude const & p1, geometry::PointWithAltitude const & p2)
  { return AlmostEqualAbs(p1, p2, kMwmPointAccuracy); });

  auto const routePoints = GetRoutePoints();
  std::string const from = GetNameFromPoint(routePoints.front());
  std::string const to = GetNameFromPoint(routePoints.back());

  return m_bmManager->SaveRoute(std::move(junctions), from, to);
}

bool RoutingManager::DisableFollowMode()
{
  bool const disabled = m_routingSession.DisableFollowMode();
  if (disabled)
  {
    m_transitReadManager->BlockTransitSchemeMode(false /* isBlocked */);
    m_drapeEngine.SafeCall(&df::DrapeEngine::DeactivateRouteFollowing);
  }
  return disabled;
}

void SessionRoute::CheckLocationForRouting(location::GpsInfo const & info)
{
  if (!m_session.IsActive())
    return;

  SessionState state = m_session.OnLocationPositionChanged(info);
  // The car has left the route for an alternative: it is followed at once instead of a rebuild.
  if (IsLeavingRoute(state) && SwapToAlternativeUnderCar(info))
    state = m_session.OnLocationPositionChanged(info);
  if (state == SessionState::RouteNeedRebuild)
  {
    m_session.RebuildRoute(mercator::FromLatLon(info.m_latitude, info.m_longitude), m_onRebuilt,
                           nullptr /* needMoreMapsCallback */, nullptr /* removeRouteCallback */,
                           RouterDelegate::kNoTimeout, SessionState::RouteRebuilding, true /* adjustToPrevRoute */);
  }
}

void SessionRoute::LeaveRoute(location::GpsInfo const & info)
{
  if (!m_session.IsActive() || !m_session.IsOnRoute())
    return;

  if (SwapToAlternativeUnderCar(info))
  {
    m_session.OnLocationPositionChanged(info);
    return;
  }
  m_session.RebuildRoute(mercator::FromLatLon(info.m_latitude, info.m_longitude), m_onRebuilt,
                         nullptr /* needMoreMapsCallback */, nullptr /* removeRouteCallback */,
                         RouterDelegate::kNoTimeout, SessionState::RouteRebuilding, true /* adjustToPrevRoute */);
}

void SessionRoute::RebuildRouteIfOffRoute(location::GpsInfo const & info)
{
  if (!m_session.IsActive())
    return;

  // A position set by hand has no track of positions behind it, so the router does not know where the car
  // goes and turns it around to the fastest way. The direction along the road is known, a track is made.
  if (info.HasBearing())
  {
    // Long enough for the track, the shorter segments are ignored.
    double constexpr kTrackLengthM = 40.0;
    m2::PointD const current = mercator::FromLatLon(info.m_latitude, info.m_longitude);
    double const angle = math::DegToRad(location::BearingToAngle(info.m_bearing));
    m2::PointD const behind =
        current - m2::PointD(std::cos(angle), std::sin(angle)) * mercator::MetersToMercator(kTrackLengthM);
    m_session.ClearPositionAccumulator();
    m_session.PushPositionAccumulator(behind);
    m_session.PushPositionAccumulator(current);
  }

  // Moves the route iterator to the closest point ahead, so the passed part of the route is cut off.
  SessionState state = m_session.OnLocationPositionChanged(info);
  // The car is on an alternative route: it is followed instead of a rebuild.
  if ((IsLeavingRoute(state) || IsAgainstRoute(info)) && SwapToAlternativeUnderCar(info))
    state = m_session.OnLocationPositionChanged(info);
  // A route just built is not started until the car is matched to it: the car set off it is off the route.
  if (state != SessionState::OnRoute && state != SessionState::RouteNotStarted &&
      state != SessionState::RouteNeedRebuild)
  {
    return;
  }

  m2::PointD const position = mercator::FromLatLon(info.m_latitude, info.m_longitude);
  // How far the car is shown from the position on the route, for the log of a drive.
  double shownAwayM = -1.0;
  if (state == SessionState::OnRoute)
  {
    location::GpsInfo matched(info);
    location::RouteMatchingInfo routeMatchingInfo;
    m_session.MatchLocationToRoute(matched, routeMatchingInfo);
    if (routeMatchingInfo.IsMatched())
      shownAwayM = mercator::DistanceOnEarth(routeMatchingInfo.GetPosition(), position);
    if (routeMatchingInfo.IsMatched() && shownAwayM <= info.m_horizontalAccuracy && !IsAgainstRoute(info))
      return;
  }

  LOG(LINFO, ("Manual position is off the route, rebuilding, bearing", info.m_bearing, "shown", shownAwayM,
              "m away on the route"));
  m_session.RebuildRoute(position, m_onRebuilt, nullptr /* needMoreMapsCallback */, nullptr /* removeRouteCallback */,
                         RouterDelegate::kNoTimeout, SessionState::RouteRebuilding, false /* adjustToPrevRoute */);
}

bool SessionRoute::SnapToRoad(ms::LatLon const & latLon, double bearingDeg, double radiusM, bool matchRoute,
                              ms::LatLon & snapped, double & snappedBearingDeg)
{
  // A part of the route farther than the road by more than this is not where the car is: the car has left the
  // route, e.g. gone straight where the route turns, and the route would pull it back to the turn.
  double constexpr kSameRoadM = 3.0;

  m2::PointD const point = mercator::FromLatLon(latLon);
  m2::PointD direction = m2::PointD::Zero();
  if (bearingDeg >= 0.0)
  {
    double const angle = math::DegToRad(location::BearingToAngle(bearingDeg));
    direction = m2::PointD(std::cos(angle), std::sin(angle));
  }
  m2::PointD projected;
  double angleRad = 0.0;
  bool found = m_session.FindRoadAlong(point, direction, radiusM, projected, angleRad);
  // The route and the road are compared as the road is chosen: by the distance and the direction. A route
  // branching away at a fork would pull the car from the road it goes straight along otherwise.
  auto const cost = [&](m2::PointD const & p, double roadAngle)
  {
    double cost = mercator::DistanceOnEarth(point, p);
    if (bearingDeg >= 0.0)
    {
      double const diffDeg =
          std::fabs(math::RadToDeg(ang::GetShortestDistance(ang::AngleTo(m2::PointD::Zero(), direction), roadAngle)));
      cost += std::min(diffDeg, 180.0 - diffDeg) * routing::IRouter::kRoadMetersPerDeg;
    }
    return cost;
  };
  double const roadCost = found ? cost(projected, angleRad) : std::numeric_limits<double>::max();

  if (matchRoute && m_session.IsActive() && m_session.IsOnRoute())
  {
    // The position is projected to the route itself. The route iterator is where the previous position was
    // matched, snapping to it would move the car back by the distance driven since on every snap.
    Route const * route = m_session.GetRoute();
    if (route != nullptr && route->IsValid() && route->GetPoly().GetPoints().size() >= 2)
    {
      auto const & points = route->GetPoly().GetPoints();
      size_t anchorIdx = 0;
      m2::PointD anchor;
      double const routeM = FindRouteAnchor(points, point, bearingDeg, anchorIdx, anchor);
      bool const atEnd = anchor == points[anchorIdx] || anchor == points[anchorIdx + 1];
      if (routeM <= radiusM && !(atEnd && routing::IRouter::IsRoadBehind(point, anchor, direction)) &&
          cost(anchor, ang::AngleTo(points[anchorIdx], points[anchorIdx + 1])) <= roadCost + kSameRoadM)
      {
        found = true;
        projected = anchor;
        angleRad = ang::AngleTo(points[anchorIdx], points[anchorIdx + 1]);
      }
    }
  }
  if (!found)
    return false;

  snapped = mercator::ToLatLon(projected);
  snappedBearingDeg = location::AngleToBearing(math::RadToDeg(angleRad));
  return true;
}

namespace
{
// The route repeats a point, e.g. where it starts from a point on a road. Such a segment has no direction.
bool IsSamePoint(m2::PointD const & a, m2::PointD const & b)
{
  double constexpr kSamePointM = 0.01;
  return mercator::DistanceOnEarth(a, b) < kSamePointM;
}

// Returns the distance in meters from |point| to the closest part of the route going the |bearingDeg| way
// (any way if it is negative), |anchorIdx| is the index of its segment and |anchor| is the closest point on it.
double FindRouteAnchor(std::vector<m2::PointD> const & points, m2::PointD const & point, double bearingDeg,
                       size_t & anchorIdx, m2::PointD & anchor)
{
  // A part of the route going another way is a street the car has already left or has not reached yet.
  double constexpr kMaxBearingDiffDeg = 60.0;

  double const carAngle = bearingDeg < 0.0 ? 0.0 : math::DegToRad(location::BearingToAngle(bearingDeg));
  double distanceToRouteM = std::numeric_limits<double>::max();
  for (size_t i = 0; i + 1 < points.size(); ++i)
  {
    if (IsSamePoint(points[i], points[i + 1]))
      continue;

    // The route passes the car several times, e.g. the street it has just turned from is still a part of
    // the route. Only a part going the way the car looks is the one the car drives along now.
    if (bearingDeg >= 0.0)
    {
      double const diffDeg =
          std::fabs(math::RadToDeg(ang::GetShortestDistance(ang::AngleTo(points[i], points[i + 1]), carAngle)));
      if (diffDeg > kMaxBearingDiffDeg)
        continue;
    }

    m2::PointD const projection = m2::ParametrizedSegment<m2::PointD>(points[i], points[i + 1]).ClosestPointTo(point);
    double const distanceM = mercator::DistanceOnEarth(projection, point);
    // At a crossing the route parts before and after it are equally close. The one before is taken: the
    // car stopped at the crossing has not passed it yet, and a turned car is told by its direction.
    if (distanceM < distanceToRouteM)
    {
      distanceToRouteM = distanceM;
      anchor = projection;
      anchorIdx = i;
    }
  }
  return distanceToRouteM;
}

// Returns the point index of the closest turn after |segIdx|, |lastIdx| if there is no turn.
size_t FindTurnAfterIdx(Route const & route, size_t segIdx, size_t lastIdx)
{
  turns::TurnItem turn;
  route.GetTurnAfterIdx(segIdx, turn);
  return std::min(static_cast<size_t>(turn.m_index), lastIdx);
}

// Returns the point index of the closest turn before |segIdx|, 0 if there is no turn.
size_t FindTurnBeforeIdx(Route const & route, size_t segIdx)
{
  turns::TurnItem turn;
  size_t found = 0;
  for (size_t idx = 0; idx <= segIdx;)
  {
    route.GetTurnAfterIdx(idx, turn);
    if (turn.m_index > segIdx)
      break;
    found = turn.m_index;
    idx = turn.m_index + 1;
  }
  return found;
}
}  // namespace

bool SessionRoute::ShiftAlongRoute(ms::LatLon const & latLon, double bearingDeg, double distanceM, ms::LatLon & shifted,
                                   double & shiftedBearingDeg, double & appliedM, bool & atCrossing)
{
  // A sharp bend of the route is a turn to another street, the position must not be shifted past it.
  double constexpr kTurnBendDeg = 45.0;
  // A position farther than this is not on the route, moving it along the route would teleport the car.
  double constexpr kMaxDistanceToRouteM = 25.0;
  // Crossings are searched a bit farther than the position can be moved.
  double constexpr kCrossingMarginM = 20.0;
  // Less than this is no movement at all.
  double constexpr kMinShiftM = 0.5;
  // The route goes through the road points, so its point at a crossing is the crossing itself.
  static double constexpr kSameCrossingM = 1.0;

  appliedM = 0.0;
  atCrossing = false;
  if (!m_session.IsActive() || !m_session.IsOnRoute())
    return false;

  Route const * route = m_session.GetRoute();
  if (route == nullptr || !route->IsValid())
    return false;

  auto const & points = route->GetPoly().GetPoints();
  if (points.size() < 2)
    return false;

  // The position is moved from where the user sees it and not from the route iterator: the iterator
  // lags behind when the position is off the route, and the car would jump back to it.
  size_t anchorIdx = 0;
  m2::PointD anchor;
  m2::PointD const point = mercator::FromLatLon(latLon);
  double distanceToRouteM = FindRouteAnchor(points, point, bearingDeg, anchorIdx, anchor);
  // No part of the route close by goes the way the car looks: the direction is a stale guess, e.g. from
  // old marks, and the car is on the route anyway, it drives the way the route goes.
  if (distanceToRouteM > kMaxDistanceToRouteM && bearingDeg >= 0.0)
    distanceToRouteM = FindRouteAnchor(points, point, kAnyBearing, anchorIdx, anchor);
  if (distanceToRouteM > kMaxDistanceToRouteM)
  {
    LOG(LINFO, ("The position is", distanceToRouteM, "m from the route, not moving it along the route"));
    return false;
  }

  bool const forward = distanceM >= 0.0;
  // The end of the current segment in the order of the movement and the direction of the movement.
  size_t to = forward ? anchorIdx + 1 : anchorIdx;
  double moveAngle = forward ? ang::AngleTo(points[anchorIdx], points[anchorIdx + 1])
                             : ang::AngleTo(points[anchorIdx + 1], points[anchorIdx]);
  m2::PointD position = anchor;
  double remainingM = std::fabs(distanceM);

  // The position must not be moved past a turn or a crossing: the car may take another street there,
  // even one the route does not go along, and the car that has already turned must not be dragged back
  // to the street it has left.
  size_t const stopIdx =
      forward ? FindTurnAfterIdx(*route, anchorIdx, points.size() - 1) : FindTurnBeforeIdx(*route, anchorIdx);
  // Crossings the route goes straight through are not turns of the route, they are taken from the roads.
  std::vector<m2::PointD> crossings;
  m_session.FindRoadCrossings(mercator::RectByCenterXYAndSizeInMeters(anchor, 2.0 * (remainingM + kCrossingMarginM)),
                              crossings);
  auto const isCrossing = [&crossings](m2::PointD const & p)
  {
    return std::any_of(crossings.cbegin(), crossings.cend(), [&p](m2::PointD const & crossing)
    { return mercator::DistanceOnEarth(crossing, p) < kSameCrossingM; });
  };

  // Why the position has stopped, for the log of a drive.
  std::string_view stopReason = "distance";
  while (remainingM > 0.0)
  {
    double const segmentM = mercator::DistanceOnEarth(position, points[to]);
    if (remainingM <= segmentM)
    {
      position += (points[to] - position) * (remainingM / segmentM);
      appliedM += remainingM;
      break;
    }
    position = points[to];
    appliedM += segmentM;
    remainingM -= segmentM;

    if (forward ? to >= stopIdx : to <= stopIdx)
    {
      // The route starts where the position was when it was built, e.g. after the previous move back.
      stopReason = !forward && to == 0 ? "start" : "turn";
      break;
    }
    if (isCrossing(position))
    {
      stopReason = "crossing";
      break;
    }

    size_t const following = forward ? to + 1 : to - 1;
    if (!IsSamePoint(points[to], points[following]))
    {
      // A turn is not crossed in either direction: the car may have taken another street there, and going
      // back through it would drag the position to a street the car is not on.
      double const nextAngle = ang::AngleTo(points[to], points[following]);
      if (std::fabs(math::RadToDeg(ang::GetShortestDistance(moveAngle, nextAngle))) > kTurnBendDeg)
      {
        stopReason = "bend";
        break;
      }
      moveAngle = nextAngle;
    }
    to = following;
  }

  shifted = mercator::ToLatLon(position);
  // The car looks along the route also when it is moved back.
  shiftedBearingDeg = location::AngleToBearing(math::RadToDeg(forward ? moveAngle : moveAngle + math::pi));
  if (!forward)
    appliedM = -appliedM;
  LOG(LINFO, ("Moved along the route by", appliedM, "of", distanceM, "m, stopped by", stopReason, "at point", to, "of",
              points.size(), "anchor", anchorIdx, distanceToRouteM, "m away, crossings", crossings.size()));
  // The car has come from behind the start of the route along the same road, so it is moved there without
  // the route. At a turn or crossing the position is on the route and must not be moved another way.
  if (stopReason == "start" && std::fabs(appliedM) < kMinShiftM)
    return false;
  atCrossing = stopReason == "turn" || stopReason == "crossing" || stopReason == "bend";
  return true;
}

bool SessionRoute::IsAgainstRoute(location::GpsInfo const & info)
{
  ms::LatLon projected;
  double routeBearingDeg;
  if (!info.HasBearing() || !ProjectToRoute(ms::LatLon(info.m_latitude, info.m_longitude), info.m_horizontalAccuracy,
                                            projected, routeBearingDeg))
  {
    return false;
  }
  double const diffDeg = std::fabs(
      math::RadToDeg(ang::GetShortestDistance(math::DegToRad(info.m_bearing), math::DegToRad(routeBearingDeg))));
  return diffDeg > 90.0;
}

bool SessionRoute::SnapToMainRoad(ms::LatLon const & latLon, double radiusM, ms::LatLon & snapped, double & bearingDeg)
{
  m2::PointD projected;
  double angleRad = 0.0;
  if (!m_session.FindMainRoad(mercator::FromLatLon(latLon), radiusM, projected, angleRad))
    return false;
  snapped = mercator::ToLatLon(projected);
  bearingDeg = location::AngleToBearing(math::RadToDeg(angleRad));
  return true;
}

std::vector<std::pair<ms::LatLon, double>> SessionRoute::FindRoadsAlong(ms::LatLon const & latLon, double bearingDeg,
                                                                        double radiusM) const
{
  double const angle = math::DegToRad(location::BearingToAngle(bearingDeg));
  std::vector<routing::RoadAlong> roads;
  m_session.FindRoadsAlong(mercator::FromLatLon(latLon), m2::PointD(std::cos(angle), std::sin(angle)), radiusM, roads);
  std::vector<std::pair<ms::LatLon, double>> result;
  result.reserve(roads.size());
  for (auto const & road : roads)
  {
    if (road.m_wrongWay)
      continue;
    result.emplace_back(mercator::ToLatLon(road.m_point), location::AngleToBearing(math::RadToDeg(road.m_angleRad)));
  }
  return result;
}

std::vector<ms::LatLon> SessionRoute::FindRoadCrossings(ms::LatLon const & center, double radiusM) const
{
  std::vector<m2::PointD> crossings;
  m_session.FindRoadCrossings(mercator::RectByCenterXYAndSizeInMeters(mercator::FromLatLon(center), 2.0 * radiusM),
                              crossings);
  std::vector<ms::LatLon> result;
  result.reserve(crossings.size());
  for (auto const & crossing : crossings)
    result.push_back(mercator::ToLatLon(crossing));
  return result;
}

bool SessionRoute::IsRoundaboutNear(ms::LatLon const & center, double radiusM) const
{
  return m_session.IsRoundaboutNear(mercator::FromLatLon(center), radiusM);
}

std::optional<double> SessionRoute::GetRouteAside(ms::LatLon const & latLon, double bearingDeg) const
{
  if (!m_session.IsActive() || !m_session.IsOnRoute())
    return {};

  Route const * route = m_session.GetRoute();
  if (route == nullptr || !route->IsValid())
    return {};

  auto const & points = route->GetPoly().GetPoints();
  if (points.size() < 2)
    return {};

  size_t anchorIdx = 0;
  m2::PointD anchor;
  m2::PointD const point = mercator::FromLatLon(latLon);
  double const distanceM = FindRouteAnchor(points, point, bearingDeg, anchorIdx, anchor);
  if (distanceM == std::numeric_limits<double>::max())
    return {};
  // The route is on the right of a car that is on the left of it.
  bool const carOnLeft = m2::CrossProduct(points[anchorIdx + 1] - points[anchorIdx], point - anchor) > 0.0;
  return carOnLeft ? distanceM : -distanceM;
}

bool SessionRoute::ProjectToRoute(ms::LatLon const & latLon, double radiusM, ms::LatLon & projected,
                                  double & bearingDeg)
{
  if (!m_session.IsActive())
    return false;

  Route const * route = m_session.GetRoute();
  if (route == nullptr || !route->IsValid())
    return false;

  auto const & points = route->GetPoly().GetPoints();
  size_t anchorIdx = 0;
  m2::PointD anchor;
  m2::PointD const point = mercator::FromLatLon(latLon);
  double const distanceToRouteM =
      points.size() < 2 ? radiusM + 1 : FindRouteAnchor(points, point, kAnyBearing, anchorIdx, anchor);
  if (distanceToRouteM > radiusM)
    return false;

  // Another road closer to the tap, e.g. a street crossing the route: the car is on it, and the route
  // would pull the position to the crossing.
  double constexpr kSameRoadM = 3.0;
  routing::EdgeProj closest;
  if (m_session.FindClosestProjectionToRoad(point, m2::PointD::Zero(), radiusM, closest) &&
      mercator::DistanceOnEarth(point, closest.m_point) + kSameRoadM < distanceToRouteM)
  {
    return false;
  }

  projected = mercator::ToLatLon(anchor);
  bearingDeg = location::AngleToBearing(math::RadToDeg(ang::AngleTo(points[anchorIdx], points[anchorIdx + 1])));
  return true;
}

void RoutingManager::CallRouteBuilded(RouterResultCode code, storage::CountriesSet const & absentCountries)
{
  CHECK(m_routingBuildingCallback, ());
  m_routingBuildingCallback(code, absentCountries);
}

void RoutingManager::MatchLocationToRoute(location::GpsInfo & location, location::RouteMatchingInfo & routeMatchingInfo)
{
  if (!IsRoutingActive())
    return;

  bool const matchedToRoute = m_routingSession.MatchLocationToRoute(location, routeMatchingInfo);

  if (!matchedToRoute && m_currentRouterType == RouterType::Vehicle)
    m_routingSession.MatchLocationToRoadGraph(location);
}

location::RouteMatchingInfo RoutingManager::GetRouteMatchingInfo(location::GpsInfo & info)
{
  m_sessionRoute.CheckLocationForRouting(info);

  location::RouteMatchingInfo routeMatchingInfo;
  MatchLocationToRoute(info, routeMatchingInfo);
  return routeMatchingInfo;
}

void RoutingManager::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine, bool is3dAllowed)
{
  m_drapeEngine.Set(engine);
  if (engine == nullptr)
    return;

  // Apply gps info which was set before drape engine creation.
  if (m_gpsInfoCache != nullptr)
  {
    auto routeMatchingInfo = GetRouteMatchingInfo(*m_gpsInfoCache);
    m_drapeEngine.SafeCall(&df::DrapeEngine::SetGpsInfo, *m_gpsInfoCache, m_routingSession.IsNavigable(),
                           routeMatchingInfo);
    m_gpsInfoCache.reset();
  }

  std::vector<std::string> symbols;
  symbols.reserve(kTransitSymbols.size() * 2);
  for (auto const & typePair : kTransitSymbols)
  {
    symbols.push_back(typePair.second + "-s");
    symbols.push_back(typePair.second + "-m");
  }
  m_drapeEngine.SafeCall(&df::DrapeEngine::RequestSymbolsSize, symbols,
                         [this, is3dAllowed](std::map<std::string, m2::PointF> && sizes)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [this, is3dAllowed, sizes = std::move(sizes)]() mutable
    {
      m_transitSymbolSizes = std::move(sizes);

      // In case of the engine reinitialization recover route.
      if (IsRoutingActive())
      {
        m_routingSession.RouteCall([this](RoutesResult const & result) { InsertRoute(result); });

        if (is3dAllowed && m_routingSession.IsFollowing())
          m_drapeEngine.SafeCall(&df::DrapeEngine::EnablePerspective);
      }
    });
  });
}

bool RoutingManager::HasRouteAltitude() const
{
  return m_loadAltitudes && m_routingSession.HasRouteAltitude();
}

bool RoutingManager::GetRouteElevationInfo(ElevationInfo & ei) const
{
  auto const * route = m_routingSession.GetRoute();
  if (!route || !route->IsValid() || !route->HaveAltitudes())
    return false;

  geometry::Altitudes altitudes;
  route->GetAltitudes(altitudes);

  ei.Assign(route->GetSegDistanceMeters(), altitudes);
  ei.Simplify();
  return true;
}

std::optional<m2::PointD> RoutingManager::GetRoutePointAtDistance(double distanceMeters) const
{
  auto const * route = m_routingSession.GetRoute();
  if (!route || !route->IsValid())
    return std::nullopt;

  auto const & distances = route->GetSegDistanceMeters();
  auto const & points = route->GetPoly().GetPoints();
  return m2::InterpolatePointAtDistance(distances, points, distanceMeters);
}

void RoutingManager::SetRouter(RouterType type)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ("SetRouter"));

  if (m_currentRouterType == type)
    return;

  // Hide preview.
  HidePreviewSegments();

  SetLastUsedRouter(type);

  // m_numMwmIDs is initialized in Init(), which runs on the GUI thread after async map loading
  // completes (Framework::InitRouting). SetRouter() can be called earlier (e.g. route restoration
  // triggers onRoutePointsLoaded → RoutingController.prepare → Router.set before maps finish loading).
  // Deferring is safe: Init() calls SetRouterImpl(GetLastUsedRouter()), picking up the type saved above.
  /// @TODO(AB): The proper fix is to not set mFrameworkInitialized=true in Android's OrganicMaps.java
  /// until the async native callback fires, so Java subsystems never see a half-initialized core.
  if (!m_numMwmIDs)
    return;

  SetRouterImpl(type);
}

// static
uint32_t RoutingManager::InvalidRoutePointsTransactionId()
{
  return kInvalidTransactionId;
}

uint32_t RoutingManager::GenerateRoutePointsTransactionId() const
{
  static uint32_t id = kInvalidTransactionId + 1;
  return id++;
}

uint32_t RoutingManager::OpenRoutePointsTransaction()
{
  auto const id = GenerateRoutePointsTransactionId();
  m_routePointsTransactions[id].m_routeMarks = GetRoutePoints();
  return id;
}

void RoutingManager::ApplyRoutePointsTransaction(uint32_t transactionId)
{
  if (m_routePointsTransactions.find(transactionId) == m_routePointsTransactions.end())
    return;

  // If we apply a transaction we can remove all earlier transactions.
  // All older transactions must be kept since they can be applied or cancelled later.
  for (auto it = m_routePointsTransactions.begin(); it != m_routePointsTransactions.end();)
    if (it->first <= transactionId)
      it = m_routePointsTransactions.erase(it);
    else
      ++it;
}

void RoutingManager::CancelRoutePointsTransaction(uint32_t transactionId)
{
  auto const it = m_routePointsTransactions.find(transactionId);
  if (it == m_routePointsTransactions.end())
    return;
  auto routeMarks = it->second.m_routeMarks;

  // If we cancel a transaction we must remove all later transactions.
  for (auto it = m_routePointsTransactions.begin(); it != m_routePointsTransactions.end();)
    if (it->first >= transactionId)
      it = m_routePointsTransactions.erase(it);
    else
      ++it;

  // Revert route points.
  ASSERT(m_bmManager != nullptr, ());
  auto editSession = m_bmManager->GetEditSession();
  editSession.ClearGroup(UserMark::Type::ROUTING);
  RoutePointsLayout routePoints(*m_bmManager);
  for (auto & markData : routeMarks)
    routePoints.AddRoutePoint(std::move(markData));
}

bool RoutingManager::HasSavedRoutePoints() const
{
  auto const fileName = GetPlatform().SettingsPathForFile(kRoutePointsFile);
  return GetPlatform().IsFileExistsByFullPath(fileName);
}

void RoutingManager::LoadRoutePoints(LoadRouteHandler const & handler)
{
  GetPlatform().RunTask(Platform::Thread::File, [this, handler]()
  {
    if (!HasSavedRoutePoints())
    {
      if (handler)
        handler(false /* success */);
      return;
    }

    // Delete file after loading.
    auto const fileName = GetPlatform().SettingsPathForFile(kRoutePointsFile);
    SCOPE_GUARD(routePointsFileGuard, std::bind(&FileWriter::DeleteFileX, std::cref(fileName)));

    std::string data;
    try
    {
      ReaderPtr<Reader>(GetPlatform().GetReader(fileName)).ReadAsString(data);
    }
    catch (RootException const & ex)
    {
      LOG(LWARNING, ("Loading road points failed:", ex.Msg()));
      if (handler)
        handler(false /* success */);
      return;
    }

    auto points = DeserializeRoutePoints(data);
    if (handler && points.empty())
    {
      handler(false /* success */);
      return;
    }

    GetPlatform().RunTask(Platform::Thread::Gui, [this, handler, points = std::move(points)]() mutable
    {
      ASSERT(m_bmManager != nullptr, ());
      // If we have found my position and the saved route used the user's position, we use my position as start point.
      bool routeUsedPosition = false;
      auto const & myPosMark = m_bmManager->MyPositionMark();
      auto editSession = m_bmManager->GetEditSession();
      editSession.ClearGroup(UserMark::Type::ROUTING);
      // Saved points have no intermediate indices; replay their order without optimization.
      for (auto & p : points)
      {
        // Check if the saved route used the user's position
        if (p.m_replaceWithMyPositionAfterRestart && p.m_pointType == RouteMarkType::Start)
          routeUsedPosition = true;

        if (p.m_replaceWithMyPositionAfterRestart && p.m_pointType == RouteMarkType::Start && myPosMark.HasPosition())
        {
          RouteMarkData startPt;
          startPt.m_pointType = RouteMarkType::Start;
          startPt.m_isMyPosition = true;
          startPt.m_position = myPosMark.GetPivot();
          AddRoutePoint(std::move(startPt), false /* optimize */);
        }
        else
        {
          AddRoutePoint(std::move(p), false /* optimize */);
        }
      }

      // If we don't have my position and the saved route used it, save loading timestamp.
      // Probably we will get my position soon.
      if (routeUsedPosition && !myPosMark.HasPosition())
        m_loadRoutePointsTimestamp = std::chrono::steady_clock::now();

      if (handler)
        handler(true /* success */);
    });
  });
}

void RoutingManager::SaveRoutePoints()
{
  auto points = GetRoutePointsToSave();
  if (points.empty())
  {
    DeleteSavedRoutePoints();
    return;
  }

  GetPlatform().RunTask(Platform::Thread::File, [points = std::move(points)]()
  {
    try
    {
      auto const fileName = GetPlatform().SettingsPathForFile(kRoutePointsFile);
      FileWriter writer(fileName);
      std::string const pointsData = SerializeRoutePoints(points);
      writer.Write(pointsData.c_str(), pointsData.length());
    }
    catch (RootException const & ex)
    {
      LOG(LWARNING, ("Saving road points failed:", ex.Msg()));
    }
  });
}

std::vector<RouteMarkData> RoutingManager::GetRoutePointsToSave() const
{
  auto points = GetRoutePoints();
  if (points.size() < 2 || points.back().m_isPassed)
    return {};

  std::vector<RouteMarkData> result;
  result.reserve(points.size());

  // Resume from the last passed point, or the original start when none was passed.
  result.emplace_back(GetLastPassedPoint(m_bmManager, points));

  for (auto & p : points)
  {
    // Here we skip passed points and the start point.
    if (p.m_isPassed || p.m_pointType == RouteMarkType::Start)
      continue;

    result.push_back(std::move(p));
  }

  if (result.size() < 2)
    return {};

  return result;
}

void RoutingManager::OnExtrapolatedLocationUpdate(location::GpsInfo const & info)
{
  location::GpsInfo gpsInfo(info);
  if (!m_drapeEngine)
    m_gpsInfoCache = std::make_unique<location::GpsInfo>(gpsInfo);

  auto routeMatchingInfo = GetRouteMatchingInfo(gpsInfo);
  m_drapeEngine.SafeCall(&df::DrapeEngine::SetGpsInfo, gpsInfo, m_routingSession.IsNavigable(), routeMatchingInfo);
}

void RoutingManager::DeleteSavedRoutePoints()
{
  if (!HasSavedRoutePoints())
    return;

  GetPlatform().RunTask(Platform::Thread::File, []()
  {
    auto const fileName = GetPlatform().SettingsPathForFile(kRoutePointsFile);
    FileWriter::DeleteFileX(fileName);
  });
}

void RoutingManager::UpdatePreviewMode()
{
  SetSubroutesVisibility(false /* visible */);
  HidePreviewSegments();
  ShowPreviewSegments(GetRoutePoints());
}

void RoutingManager::CancelPreviewMode()
{
  SetSubroutesVisibility(true /* visible */);
  HidePreviewSegments();
}

m2::RectD RoutingManager::ShowPreviewSegments(std::vector<RouteMarkData> const & routePoints)
{
  df::DrapeEngineLockGuard lock(m_drapeEngine);
  if (!lock)
    return mercator::Bounds::FullRect();

  m2::RectD rect;
  for (size_t pointIndex = 0; pointIndex + 1 < routePoints.size(); pointIndex++)
  {
    rect.Add(routePoints[pointIndex].m_position);
    rect.Add(routePoints[pointIndex + 1].m_position);
    lock.Get()->AddRoutePreviewSegment(routePoints[pointIndex].m_position, routePoints[pointIndex + 1].m_position);
  }
  return rect;
}

void RoutingManager::HidePreviewSegments()
{
  m_drapeEngine.SafeCall(&df::DrapeEngine::RemoveAllRoutePreviewSegments);
}

void RoutingManager::CancelRecommendation(Recommendation recommendation)
{
  if (recommendation == Recommendation::RebuildAfterPointsLoading)
    m_loadRoutePointsTimestamp = std::chrono::steady_clock::time_point();
}

TransitRouteInfo RoutingManager::GetTransitRouteInfo() const
{
  std::lock_guard<std::mutex> lock(m_drapeSubroutesMutex);
  return m_transitRouteInfo;
}

void RoutingManager::SetSubroutesVisibility(bool visible)
{
  df::DrapeEngineLockGuard lock(m_drapeEngine);
  if (!lock)
    return;

  std::lock_guard<std::mutex> lockSubroutes(m_drapeSubroutesMutex);
  for (auto const & subrouteId : m_drapeSubroutes)
    lock.Get()->SetSubrouteVisibility(subrouteId, visible);
}

bool RoutingManager::IsSpeedCamLimitExceeded() const
{
  return m_routingSession.IsSpeedCamLimitExceeded();
}
