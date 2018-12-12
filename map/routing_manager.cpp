#include "routing_manager.hpp"

#include "map/chart_generator.hpp"
#include "map/routing_mark.hpp"

#include "private.h"

#include "tracking/reporter.hpp"

#include "routing/checkpoint_predictor.hpp"
#include "routing/index_router.hpp"
#include "routing/online_absent_fetcher.hpp"
#include "routing/route.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/speed_camera.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "drape_frontend/drape_engine.hpp"

#include "indexer/map_style_reader.hpp"
#include "indexer/scales.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/routing_helpers.hpp"
#include "storage/storage.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/string_utf8_multilang.hpp"

#include "platform/country_file.hpp"
#include "platform/mwm_traits.hpp"
#include "platform/platform.hpp"
#include "platform/socket.hpp"

#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

#include "3party/Alohalytics/src/alohalytics.h"
#include "3party/jansson/myjansson.hpp"

#include <iomanip>
#include <ios>
#include <map>
#include <sstream>

using namespace routing;
using namespace std;

namespace
{
char const kRouterTypeKey[] = "router";

double const kRouteScaleMultiplier = 1.5;

string const kRoutePointsFile = "route_points.dat";

uint32_t constexpr kInvalidTransactionId = 0;

void FillTurnsDistancesForRendering(vector<RouteSegment> const & segments,
                                    double baseDistance, vector<double> & turns)
{
  using namespace routing::turns;
  turns.clear();
  turns.reserve(segments.size());
  for (auto const & s : segments)
  {
    auto const & t = s.GetTurn();
    CHECK_NOT_EQUAL(t.m_turn, CarDirection::Count, ());
    // We do not render some of turn directions.
    if (t.m_turn == CarDirection::None || t.m_turn == CarDirection::StartAtEndOfStreet ||
        t.m_turn == CarDirection::StayOnRoundAbout || t.m_turn == CarDirection::ReachedYourDestination)
    {
      continue;
    }
    turns.push_back(s.GetDistFromBeginningMerc() - baseDistance);
  }
}

void FillTrafficForRendering(vector<RouteSegment> const & segments,
                             vector<traffic::SpeedGroup> & traffic)
{
  traffic.clear();
  traffic.reserve(segments.size());
  for (auto const & s : segments)
    traffic.push_back(s.GetTraffic());
}

RouteMarkData GetLastPassedPoint(BookmarkManager * bmManager, vector<RouteMarkData> const & points)
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
  }

  return data;
}

void SerializeRoutePoint(json_t * node, RouteMarkData const & data)
{
  ASSERT(node != nullptr, ());
  ToJSONObject(*node, "type", static_cast<int>(data.m_pointType));
  ToJSONObject(*node, "title", data.m_title);
  ToJSONObject(*node, "subtitle", data.m_subTitle);
  ToJSONObject(*node, "x", data.m_position.x);
  ToJSONObject(*node, "y", data.m_position.y);
}

RouteMarkData DeserializeRoutePoint(json_t * node)
{
  ASSERT(node != nullptr, ());
  RouteMarkData data;

  int type = 0;
  FromJSONObject(node, "type", type);
  data.m_pointType = static_cast<RouteMarkType>(type);

  FromJSONObject(node, "title", data.m_title);
  FromJSONObject(node, "subtitle", data.m_subTitle);

  FromJSONObject(node, "x", data.m_position.x);
  FromJSONObject(node, "y", data.m_position.y);

  return data;
}

string SerializeRoutePoints(vector<RouteMarkData> const & points)
{
  ASSERT_GREATER_OR_EQUAL(points.size(), 2, ());
  auto pointsNode = base::NewJSONArray();
  for (auto const & p : points)
  {
    auto pointNode = base::NewJSONObject();
    SerializeRoutePoint(pointNode.get(), p);
    json_array_append_new(pointsNode.get(), pointNode.release());
  }
  unique_ptr<char, JSONFreeDeleter> buffer(
    json_dumps(pointsNode.get(), JSON_COMPACT | JSON_ENSURE_ASCII));
  return string(buffer.get());
}

vector<RouteMarkData> DeserializeRoutePoints(string const & data)
{
  try
  {
    base::Json root(data.c_str());

    if (root.get() == nullptr || !json_is_array(root.get()))
      return {};

    size_t const sz = json_array_size(root.get());
    if (sz == 0)
      return {};

    vector<RouteMarkData> result;
    result.reserve(sz);
    for (size_t i = 0; i < sz; ++i)
    {
      auto pointNode = json_array_get(root.get(), i);
      if (pointNode == nullptr)
        continue;

      auto point = DeserializeRoutePoint(pointNode);
      if (point.m_position.EqualDxDy(m2::PointD::Zero(), 1e-7))
        continue;

      result.push_back(move(point));
    }

    if (result.size() < 2)
      return {};

    return result;
  }
  catch (base::Json::Exception const &)
  {
    return {};
  }
}

VehicleType GetVehicleType(RouterType routerType)
{
  switch (routerType)
  {
  case RouterType::Pedestrian: return VehicleType::Pedestrian;
  case RouterType::Bicycle: return VehicleType::Bicycle;
  case RouterType::Vehicle:
  case RouterType::Taxi: return VehicleType::Car;
  case RouterType::Transit: return VehicleType::Transit;
  case RouterType::Count: CHECK(false, ("Invalid type", routerType)); return VehicleType::Count;
  }
  UNREACHABLE();
}
}  // namespace

namespace marketing
{
char const * const kRoutingCalculatingRoute = "Routing_CalculatingRoute";
}  // namespace marketing

RoutingManager::RoutingManager(Callbacks && callbacks, Delegate & delegate)
  : m_callbacks(move(callbacks))
  , m_delegate(delegate)
  , m_trackingReporter(platform::CreateSocket(), TRACKING_REALTIME_HOST, TRACKING_REALTIME_PORT,
                       tracking::Reporter::kPushDelayMs)
  , m_extrapolator(
        [this](location::GpsInfo const & gpsInfo) { this->OnExtrapolatedLocationUpdate(gpsInfo); })
{
  auto const routingStatisticsFn = [](map<string, string> const & statistics) {
    alohalytics::LogEvent("Routing_CalculatingRoute", statistics);
    GetPlatform().GetMarketingService().SendMarketingEvent(marketing::kRoutingCalculatingRoute, {});
  };

  m_routingSession.Init(routingStatisticsFn,
#ifdef SHOW_ROUTE_DEBUG_MARKS
                        [this](m2::PointD const & pt) {
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

  m_routingSession.SetRoutingCallbacks(
      [this](Route const & route, RouterResultCode code) { OnBuildRouteReady(route, code); },
      [this](Route const & route, RouterResultCode code) { OnRebuildRouteReady(route, code); },
      [this](uint64_t routeId, vector<string> const & absentCountries) {
        OnNeedMoreMaps(routeId, absentCountries);
      },
      [this](RouterResultCode code) { OnRemoveRoute(code); });

  m_routingSession.SetCheckpointCallback([this](size_t passedCheckpointIdx)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [this, passedCheckpointIdx]()
    {
      size_t const pointsCount = GetRoutePointsCount();

      // TODO(@bykoianko). Since routing system may invoke callbacks from different threads and here
      // we have to use gui thread, ASSERT is not correct. Uncomment it and delete condition after
      // refactoring of threads usage in routing system.
      //ASSERT_LESS(passedCheckpointIdx, pointsCount, ());
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
      auto editSession = m_bmManager->GetEditSession();
      auto mark = editSession.CreateUserMark<SpeedCameraMark>(point);

      mark->SetIndex(0);
      if (cameraSpeedKmPH == SpeedCameraOnRoute::kNoSpeedInfo)
        return;

      double speed = cameraSpeedKmPH;
      measurement_utils::Units units = measurement_utils::Units::Metric;
      if (!settings::Get(settings::kMeasurementUnits, units))
        units = measurement_utils::Units::Metric;

      if (units == measurement_utils::Units::Imperial)
        speed = measurement_utils::KmphToMph(cameraSpeedKmPH);

      mark->SetTitle(strings::to_string(static_cast<int>(speed + 0.5)));
    });
  });

  m_routingSession.SetSpeedCamClearCallback([this]()
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [this]()
    {
      m_bmManager->GetEditSession().ClearGroup(UserMark::Type::SPEED_CAM);
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

void RoutingManager::OnBuildRouteReady(Route const & route, RouterResultCode code)
{
  // @TODO(bykoianko) Remove |code| from callback signature.
  CHECK_EQUAL(code, RouterResultCode::NoError, ());
  HidePreviewSegments();

  InsertRoute(route);
  m_drapeEngine.SafeCall(&df::DrapeEngine::StopLocationFollow);

  // Validate route (in case of bicycle routing it can be invalid).
  ASSERT(route.IsValid(), ());
  if (route.IsValid())
  {
    m2::RectD routeRect = route.GetPoly().GetLimitRect();
    routeRect.Scale(kRouteScaleMultiplier);
    m_drapeEngine.SafeCall(&df::DrapeEngine::SetModelViewRect, routeRect,
                           true /* applyRotation */, -1 /* zoom */, true /* isAnim */);
  }

  CallRouteBuilded(code, storage::TCountriesVec());
}

void RoutingManager::OnRebuildRouteReady(Route const & route, RouterResultCode code)
{
  HidePreviewSegments();

  if (code != RouterResultCode::NoError)
    return;

  InsertRoute(route);
  CallRouteBuilded(code, storage::TCountriesVec());
}

void RoutingManager::OnNeedMoreMaps(uint64_t routeId, vector<string> const & absentCountries)
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
  CallRouteBuilded(code, vector<string>());
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

RouterType RoutingManager::GetBestRouter(m2::PointD const & startPoint,
                                         m2::PointD const & finalPoint) const
{
  // todo Implement something more sophisticated here (or delete the method).
  return GetLastUsedRouter();
}

RouterType RoutingManager::GetLastUsedRouter() const
{
  string routerTypeStr;
  if (!settings::Get(kRouterTypeKey, routerTypeStr))
    return RouterType::Vehicle;

  auto const routerType = FromString(routerTypeStr);

  switch (routerType)
  {
  case RouterType::Pedestrian:
  case RouterType::Bicycle:
  case RouterType::Transit: return routerType;
  default: return RouterType::Vehicle;
  }
}

void RoutingManager::SetRouterImpl(RouterType type)
{
  auto const dataSourceGetterFn = m_callbacks.m_dataSourceGetter;
  CHECK(dataSourceGetterFn, ("Type:", type));

  VehicleType const vehicleType = GetVehicleType(type);

  m_loadAltitudes = vehicleType != VehicleType::Car;

  auto const countryFileGetter = [this](m2::PointD const & p) -> string {
    // TODO (@gorshenin): fix CountryInfoGetter to return CountryFile
    // instances instead of plain strings.
    return m_callbacks.m_countryInfoGetter().GetRegionCountryId(p);
  };

  auto numMwmIds = make_shared<NumMwmIds>();
  m_delegate.RegisterCountryFilesOnRoute(numMwmIds);

  auto & dataSource = m_callbacks.m_dataSourceGetter();

  auto localFileChecker = [this](string const & countryFile) -> bool {
    MwmSet::MwmId const mwmId = m_callbacks.m_dataSourceGetter().GetMwmIdByCountryFile(
      platform::CountryFile(countryFile));
    if (!mwmId.IsAlive())
      return false;

    return version::MwmTraits(mwmId.GetInfo()->m_version).HasRoutingIndex();
  };

  auto const getMwmRectByName = [this](string const & countryId) -> m2::RectD {
    return m_callbacks.m_countryInfoGetter().GetLimitRectForLeaf(countryId);
  };

  auto fetcher = make_unique<OnlineAbsentCountriesFetcher>(countryFileGetter, localFileChecker);
  auto router = make_unique<IndexRouter>(vehicleType, m_loadAltitudes, m_callbacks.m_countryParentNameGetterFn,
                                         countryFileGetter, getMwmRectByName, numMwmIds,
                                         MakeNumMwmTree(*numMwmIds, m_callbacks.m_countryInfoGetter()),
                                         m_routingSession, dataSource);

  m_routingSession.SetRoutingSettings(GetRoutingSettings(vehicleType));
  m_routingSession.SetRouter(move(router), move(fetcher));
  m_currentRouterType = type;
}

void RoutingManager::RemoveRoute(bool deactivateFollowing)
{
  GetPlatform().RunTask(Platform::Thread::Gui, [this, deactivateFollowing]()
  {
    m_bmManager->GetEditSession().ClearGroup(UserMark::Type::TRANSIT);
    m_bmManager->GetEditSession().ClearGroup(UserMark::Type::SPEED_CAM);

    if (deactivateFollowing)
      SetPointsFollowingMode(false /* enabled */);
  });

  if (deactivateFollowing)
  {
    m_transitReadManager->BlockTransitSchemeMode(false /* isBlocked */);
    // Remove all subroutes.
    m_drapeEngine.SafeCall(&df::DrapeEngine::RemoveSubroute,
                           dp::DrapeID(), true /* deactivateFollowing */);
  }
  else
  {
    df::DrapeEngineLockGuard lock(m_drapeEngine);
    if (lock)
    {
      lock_guard<mutex> lockSubroutes(m_drapeSubroutesMutex);
      for (auto const & subrouteId : m_drapeSubroutes)
        lock.Get()->RemoveSubroute(subrouteId, false /* deactivateFollowing */);
    }
  }

  {
    lock_guard<mutex> lock(m_drapeSubroutesMutex);
    m_drapeSubroutes.clear();
    m_transitRouteInfo = TransitRouteInfo();
  }
}

void RoutingManager::InsertRoute(Route const & route)
{
  if (!m_drapeEngine)
    return;

  // TODO: Now we always update whole route, so we need to remove previous one.
  RemoveRoute(false /* deactivateFollowing */);

  shared_ptr<TransitRouteDisplay> transitRouteDisplay;
  auto numMwmIds = make_shared<NumMwmIds>();
  if (m_currentRouterType == RouterType::Transit)
  {
    m_delegate.RegisterCountryFilesOnRoute(numMwmIds);
    auto getMwmId = [this, numMwmIds](routing::NumMwmId numMwmId)
    {
      return m_callbacks.m_dataSourceGetter().GetMwmIdByCountryFile(numMwmIds->GetFile(numMwmId));
    };
    transitRouteDisplay = make_shared<TransitRouteDisplay>(*m_transitReadManager, getMwmId,
                                                           m_callbacks.m_stringsBundleGetter,
                                                           m_bmManager, m_transitSymbolSizes);
  }

  vector<RouteSegment> segments;
  vector<m2::PointD> points;
  double distance = 0.0;

  auto const subroutesCount = route.GetSubrouteCount();
  for (size_t subrouteIndex = route.GetCurrentSubrouteIdx(); subrouteIndex < subroutesCount; ++subrouteIndex)
  {
    route.GetSubrouteInfo(subrouteIndex, segments);

    // Fill points.
    double const currentBaseDistance = distance;
    auto subroute = make_unique_dp<df::Subroute>();
    subroute->m_baseDistance = currentBaseDistance;
    subroute->m_baseDepthIndex = static_cast<double>(subroutesCount - subrouteIndex - 1);
    auto const startPt = route.GetSubrouteAttrs(subrouteIndex).GetStart().GetPoint();
    if (m_currentRouterType != RouterType::Transit)
    {
      points.clear();
      points.reserve(segments.size() + 1);
      points.push_back(startPt);
      for (auto const & s : segments)
        points.push_back(s.GetJunction().GetPoint());
      if (points.size() < 2)
      {
        LOG(LWARNING, ("Invalid subroute. Points number =", points.size()));
        continue;
      }
      subroute->m_polyline = m2::PolylineD(points);
    }
    else
    {
      subroute->m_polyline.Add(startPt);
    }
    distance = segments.back().GetDistFromBeginningMerc();

    switch (m_currentRouterType)
    {
      case RouterType::Vehicle:
      case RouterType::Taxi:
        {
          subroute->m_routeType = m_currentRouterType == RouterType::Vehicle ?
                                  df::RouteType::Car : df::RouteType::Taxi;
          subroute->AddStyle(df::SubrouteStyle(df::kRouteColor, df::kRouteOutlineColor));
          FillTrafficForRendering(segments, subroute->m_traffic);
          FillTurnsDistancesForRendering(segments, subroute->m_baseDistance,
                                         subroute->m_turns);
          break;
        }
      case RouterType::Transit:
        {
          subroute->m_routeType = df::RouteType::Transit;
          transitRouteDisplay->ProcessSubroute(segments, *subroute.get());

          if (subroute->m_polyline.GetSize() < 2)
          {
            LOG(LWARNING, ("Invalid transit subroute. Points number =", subroute->m_polyline.GetSize()));
            continue;
          }
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
          FillTurnsDistancesForRendering(segments, subroute->m_baseDistance,
                                         subroute->m_turns);
          break;
        }
      default: ASSERT(false, ("Unknown router type"));
    }

    auto const subrouteId = m_drapeEngine.SafeCallWithResult(&df::DrapeEngine::AddSubroute,
                                                             df::SubrouteConstPtr(subroute.release()));

    // TODO: we will send subrouteId to routing subsystem when we can partly update route.
    //route.SetSubrouteUid(subrouteIndex, static_cast<SubrouteUid>(subrouteId));
    lock_guard<mutex> lock(m_drapeSubroutesMutex);
    m_drapeSubroutes.push_back(subrouteId);
  }

  {
    lock_guard<mutex> lock(m_drapeSubroutesMutex);
    m_transitRouteInfo = m_currentRouterType == RouterType::Transit ? transitRouteDisplay->GetRouteInfo()
                                                                    : TransitRouteInfo();
  }

  if (m_currentRouterType == RouterType::Transit)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [transitRouteDisplay = move(transitRouteDisplay)]()
    {
      transitRouteDisplay->CreateTransitMarks();
    });
  }
}

void RoutingManager::FollowRoute()
{
  if (!m_routingSession.EnableFollowMode())
    return;

  m_transitReadManager->BlockTransitSchemeMode(true /* isBlocked */);

  // Switching on the extrapolatior only for following mode in car and bicycle navigation.
  m_extrapolator.Enable(m_currentRouterType == RouterType::Vehicle ||
                        m_currentRouterType == RouterType::Bicycle);
  m_delegate.OnRouteFollow(m_currentRouterType);

  HideRoutePoint(RouteMarkType::Start);
  SetPointsFollowingMode(true /* enabled */);

  CancelRecommendation(Recommendation::RebuildAfterPointsLoading);
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
  return mark != nullptr ? mark->IsMyPosition() : false;
}

vector<RouteMarkData> RoutingManager::GetRoutePoints() const
{
  vector<RouteMarkData> result;
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

  return m_bmManager->GetUserMarkIds(UserMark::Type::ROUTING).size()
    < RoutePointsLayout::kMaxIntermediatePointsCount + 2;
}

void RoutingManager::AddRoutePoint(RouteMarkData && markData)
{
  ASSERT(m_bmManager != nullptr, ());
  RoutePointsLayout routePoints(*m_bmManager);

  // Always replace start and finish points.
  if (markData.m_pointType == RouteMarkType::Start || markData.m_pointType == RouteMarkType::Finish)
    routePoints.RemoveRoutePoint(markData.m_pointType);

  if (markData.m_isMyPosition)
  {
    RouteMarkPoint const * mark = routePoints.GetMyPositionPoint();
    if (mark != nullptr)
      routePoints.RemoveRoutePoint(mark->GetRoutePointType(), mark->GetIntermediateIndex());
  }

  markData.m_isVisible = !markData.m_isMyPosition;
  routePoints.AddRoutePoint(move(markData));
  ReorderIntermediatePoints();
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

void RoutingManager::MoveRoutePoint(RouteMarkType currentType, size_t currentIntermediateIndex,
                                    RouteMarkType targetType, size_t targetIntermediateIndex)
{
  ASSERT(m_bmManager != nullptr, ());
  RoutePointsLayout routePoints(*m_bmManager);
  routePoints.MoveRoutePoint(currentType, currentIntermediateIndex,
                             targetType, targetIntermediateIndex);
}

void RoutingManager::MoveRoutePoint(size_t currentIndex, size_t targetIndex)
{
  ASSERT(m_bmManager != nullptr, ());

  RoutePointsLayout routePoints(*m_bmManager);
  size_t const sz = routePoints.GetRoutePointsCount();
  auto const convertIndex = [sz](RouteMarkType & type, size_t & index) {
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

void RoutingManager::SetPointsFollowingMode(bool enabled)
{
  ASSERT(m_bmManager != nullptr, ());
  RoutePointsLayout routePoints(*m_bmManager);
  routePoints.SetFollowingMode(enabled);
}

void RoutingManager::ReorderIntermediatePoints()
{
  vector<RouteMarkPoint *> prevPoints;
  vector<m2::PointD> prevPositions;
  prevPoints.reserve(RoutePointsLayout::kMaxIntermediatePointsCount);
  prevPositions.reserve(RoutePointsLayout::kMaxIntermediatePointsCount);
  RoutePointsLayout routePoints(*m_bmManager);

  RouteMarkPoint * addedPoint = nullptr;
  m2::PointD addedPosition;

  for (auto const & p : routePoints.GetRoutePoints())
  {
    CHECK(p, ());
    if (p->GetRoutePointType() == RouteMarkType::Intermediate)
    {
      // Note. An added (new) intermediate point is the first intermediate point at |routePoints.GetRoutePoints()|.
      // The other intermediate points are former ones.
      if (addedPoint == nullptr)
      {
        addedPoint = p;
        addedPosition = p->GetPivot();
      }
      else
      {
        prevPoints.push_back(p);
        prevPositions.push_back(p->GetPivot());
      }
    }
  }
  if (addedPoint == nullptr)
    return;

  CheckpointPredictor predictor(m_routingSession.GetStartPoint(), m_routingSession.GetEndPoint());

  size_t const insertIndex = predictor.PredictPosition(prevPositions, addedPosition);
  addedPoint->SetIntermediateIndex(insertIndex);
  for (size_t i = 0; i < prevPoints.size(); ++i)
    prevPoints[i]->SetIntermediateIndex(i < insertIndex ? i : i + 1);
}

void RoutingManager::GenerateNotifications(vector<string> & turnNotifications)
{
  if (m_currentRouterType == RouterType::Taxi)
    return;

  m_routingSession.GenerateNotifications(turnNotifications);
}

void RoutingManager::BuildRoute(uint32_t timeoutSec)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ("BuildRoute"));

  m_bmManager->GetEditSession().ClearGroup(UserMark::Type::TRANSIT);

  auto routePoints = GetRoutePoints();
  if (routePoints.size() < 2)
  {
    CallRouteBuilded(RouterResultCode::Cancelled, storage::TCountriesVec());
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
      CallRouteBuilded(RouterResultCode::NoCurrentPosition, storage::TCountriesVec());
      return;
    }
    p.m_position = myPosition.GetPivot();
  }

  // Check for equal points.
  double const kEps = 1e-7;
  for (size_t i = 0; i < routePoints.size(); i++)
  {
    for (size_t j = i + 1; j < routePoints.size(); j++)
    {
      if (routePoints[i].m_position.EqualDxDy(routePoints[j].m_position, kEps))
      {
        CallRouteBuilded(RouterResultCode::Cancelled, storage::TCountriesVec());
        CloseRouting(false /* remove route points */);
        return;
      }
    }
  }

  bool const isP2P = !routePoints.front().m_isMyPosition && !routePoints.back().m_isMyPosition;

  // Send tag to Push Woosh.
  {
    string tag;
    switch (m_currentRouterType)
    {
    case RouterType::Vehicle:
      tag = isP2P ? marketing::kRoutingP2PVehicleDiscovered : marketing::kRoutingVehicleDiscovered;
      break;
    case RouterType::Pedestrian:
      tag = isP2P ? marketing::kRoutingP2PPedestrianDiscovered
                  : marketing::kRoutingPedestrianDiscovered;
      break;
    case RouterType::Bicycle:
      tag = isP2P ? marketing::kRoutingP2PBicycleDiscovered : marketing::kRoutingBicycleDiscovered;
      break;
    case RouterType::Taxi:
      tag = isP2P ? marketing::kRoutingP2PTaxiDiscovered : marketing::kRoutingTaxiDiscovered;
      break;
    case RouterType::Transit:
      tag = isP2P ? marketing::kRoutingP2PTransitDiscovered : marketing::kRoutingTransitDiscovered;
      break;
    case RouterType::Count: CHECK(false, ("Bad router type", m_currentRouterType));
    }
    GetPlatform().GetMarketingService().SendPushWooshTag(tag);
  }

  CallRouteBuildStart(routePoints);

  if (IsRoutingActive())
    CloseRouting(false /* remove route points */);

  // Show preview.
  m2::RectD rect = ShowPreviewSegments(routePoints);
  rect.Scale(kRouteScaleMultiplier);
  m_drapeEngine.SafeCall(&df::DrapeEngine::SetModelViewRect, rect, true /* applyRotation */,
                         -1 /* zoom */, true /* isAnim */);

  m_routingSession.SetUserCurrentPosition(routePoints.front().m_position);

  vector<m2::PointD> points;
  points.reserve(routePoints.size());
  for (auto const & point : routePoints)
    points.push_back(point.m_position);

  m_routingSession.BuildRoute(Checkpoints(move(points)), timeoutSec);
}

void RoutingManager::SetUserCurrentPosition(m2::PointD const & position)
{
  if (IsRoutingActive())
    m_routingSession.SetUserCurrentPosition(position);

  if (m_routeRecommendCallback != nullptr)
  {
    // Check if we've found my position almost immediately after route points loading.
    auto constexpr kFoundLocationInterval = 2.0;
    auto const elapsed = chrono::steady_clock::now() - m_loadRoutePointsTimestamp;
    auto const sec = chrono::duration_cast<chrono::duration<double>>(elapsed).count();
    if (sec <= kFoundLocationInterval)
    {
      m_routeRecommendCallback(Recommendation::RebuildAfterPointsLoading);
      CancelRecommendation(Recommendation::RebuildAfterPointsLoading);
    }
  }
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

void RoutingManager::CheckLocationForRouting(location::GpsInfo const & info)
{
  if (!IsRoutingActive())
    return;

  RoutingSession::State const state = m_routingSession.OnLocationPositionChanged(info);
  if (state == RoutingSession::RouteNeedRebuild)
  {
    m_routingSession.RebuildRoute(
        MercatorBounds::FromLatLon(info.m_latitude, info.m_longitude),
        [this](Route const & route, RouterResultCode code) { OnRebuildRouteReady(route, code); },
        nullptr /* needMoreMapsCallback */, nullptr /* removeRouteCallback */, 0 /* timeoutSec */,
        RoutingSession::State::RouteRebuilding, true /* adjustToPrevRoute */);
  }
}

void RoutingManager::CallRouteBuilded(RouterResultCode code,
                                      storage::TCountriesVec const & absentCountries)
{
  m_routingBuildingCallback(code, absentCountries);
}

void RoutingManager::CallRouteBuildStart(std::vector<RouteMarkData> const & points)
{
  m_routingStartBuildCallback(points);
}

void RoutingManager::MatchLocationToRoute(location::GpsInfo & location,
                                          location::RouteMatchingInfo & routeMatchingInfo) const
{
  if (!IsRoutingActive())
    return;

  m_routingSession.MatchLocationToRoute(location, routeMatchingInfo);
}

location::RouteMatchingInfo RoutingManager::GetRouteMatchingInfo(location::GpsInfo & info)
{
  location::RouteMatchingInfo routeMatchingInfo;
  CheckLocationForRouting(info);
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
    m_drapeEngine.SafeCall(&df::DrapeEngine::SetGpsInfo, *m_gpsInfoCache,
                           m_routingSession.IsNavigable(), routeMatchingInfo);
    m_gpsInfoCache.reset();
  }

  vector<string> symbols;
  symbols.reserve(kTransitSymbols.size() * 2);
  for (auto const & typePair : kTransitSymbols)
  {
    symbols.push_back(typePair.second + "-s");
    symbols.push_back(typePair.second + "-m");
  }
  m_drapeEngine.SafeCall(&df::DrapeEngine::RequestSymbolsSize, symbols,
                         [this, is3dAllowed](map<string, m2::PointF> && sizes)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [this, is3dAllowed, sizes = move(sizes)]() mutable
    {
      m_transitSymbolSizes = move(sizes);

      // In case of the engine reinitialization recover route.
      if (IsRoutingActive())
      {
        m_routingSession.RouteCall([this](Route const & route) { InsertRoute(route); });

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

bool RoutingManager::GetRouteAltitudesAndDistancesM(vector<double> & routePointDistanceM,
                                                    feature::TAltitudes & altitudes) const
{
  if (!m_routingSession.GetRouteAltitudesAndDistancesM(routePointDistanceM, altitudes))
    return false;

  routePointDistanceM.insert(routePointDistanceM.begin(), 0.0);
  return true;
}

bool RoutingManager::GenerateRouteAltitudeChart(uint32_t width, uint32_t height,
                                                feature::TAltitudes const & altitudes,
                                                vector<double> const & routePointDistanceM,
                                                vector<uint8_t> & imageRGBAData,
                                                int32_t & minRouteAltitude,
                                                int32_t & maxRouteAltitude,
                                                measurement_utils::Units & altitudeUnits) const
{
  CHECK_EQUAL(altitudes.size(), routePointDistanceM.size(), ());
  if (altitudes.empty())
    return false;

  if (!maps::GenerateChart(width, height, routePointDistanceM, altitudes,
                           GetStyleReader().GetCurrentStyle(), imageRGBAData))
    return false;

  auto const minMaxIt = minmax_element(altitudes.cbegin(), altitudes.cend());
  feature::TAltitude const minRouteAltitudeM = *minMaxIt.first;
  feature::TAltitude const maxRouteAltitudeM = *minMaxIt.second;

  if (!settings::Get(settings::kMeasurementUnits, altitudeUnits))
    altitudeUnits = measurement_utils::Units::Metric;

  switch (altitudeUnits)
  {
  case measurement_utils::Units::Imperial:
    minRouteAltitude = measurement_utils::MetersToFeet(minRouteAltitudeM);
    maxRouteAltitude = measurement_utils::MetersToFeet(maxRouteAltitudeM);
    break;
  case measurement_utils::Units::Metric:
    minRouteAltitude = minRouteAltitudeM;
    maxRouteAltitude = maxRouteAltitudeM;
    break;
  }
  return true;
}

bool RoutingManager::IsTrackingReporterEnabled() const
{
  if (m_currentRouterType != RouterType::Vehicle)
    return false;

  if (!m_routingSession.IsFollowing())
    return false;

  bool enableTracking = true;
  settings::TryGet(tracking::Reporter::kEnableTrackingKey, enableTracking);

  return enableTracking;
}

void RoutingManager::SetRouter(RouterType type)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ("SetRouter"));

  if (m_currentRouterType == type)
    return;

  // Hide preview.
  HidePreviewSegments();

  SetLastUsedRouter(type);
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
  {
    if (it->first <= transactionId)
      it = m_routePointsTransactions.erase(it);
    else
      ++it;
  }
}

void RoutingManager::CancelRoutePointsTransaction(uint32_t transactionId)
{
  auto const it = m_routePointsTransactions.find(transactionId);
  if (it == m_routePointsTransactions.end())
    return;
  auto routeMarks = it->second.m_routeMarks;

  // If we cancel a transaction we must remove all later transactions.
  for (auto it = m_routePointsTransactions.begin(); it != m_routePointsTransactions.end();)
  {
    if (it->first >= transactionId)
      it = m_routePointsTransactions.erase(it);
    else
      ++it;
  }

  // Revert route points.
  ASSERT(m_bmManager != nullptr, ());
  auto editSession = m_bmManager->GetEditSession();
  editSession.ClearGroup(UserMark::Type::ROUTING);
  RoutePointsLayout routePoints(*m_bmManager);
  for (auto & markData : routeMarks)
    routePoints.AddRoutePoint(move(markData));
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
    SCOPE_GUARD(routePointsFileGuard, bind(&FileWriter::DeleteFileX, cref(fileName)));

    string data;
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

    GetPlatform().RunTask(Platform::Thread::Gui,
                          [this, handler, points = move(points)]() mutable
    {
      ASSERT(m_bmManager != nullptr, ());
      // If we have found my position, we use my position as start point.
      auto const & myPosMark = m_bmManager->MyPositionMark();
      auto editSession = m_bmManager->GetEditSession();
      editSession.ClearGroup(UserMark::Type::ROUTING);
      for (auto & p : points)
      {
        if (p.m_pointType == RouteMarkType::Start && myPosMark.HasPosition())
        {
          RouteMarkData startPt;
          startPt.m_pointType = RouteMarkType::Start;
          startPt.m_isMyPosition = true;
          startPt.m_position = myPosMark.GetPivot();
          AddRoutePoint(move(startPt));
        }
        else
        {
          AddRoutePoint(move(p));
        }
      }

      // If we don't have my position, save loading timestamp. Probably
      // we will get my position soon.
      if (!myPosMark.HasPosition())
        m_loadRoutePointsTimestamp = chrono::steady_clock::now();

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

  GetPlatform().RunTask(Platform::Thread::File, [points = move(points)]()
  {
    try
    {
      auto const fileName = GetPlatform().SettingsPathForFile(kRoutePointsFile);
      FileWriter writer(fileName);
      string const pointsData = SerializeRoutePoints(points);
      writer.Write(pointsData.c_str(), pointsData.length());
    }
    catch (RootException const & ex)
    {
      LOG(LWARNING, ("Saving road points failed:", ex.Msg()));
    }
  });
}

vector<RouteMarkData> RoutingManager::GetRoutePointsToSave() const
{
  auto points = GetRoutePoints();
  if (points.size() < 2 || points.back().m_isPassed)
    return {};

  vector<RouteMarkData> result;
  result.reserve(points.size());

  // Save last passed point. It will be used on points loading if my position
  // isn't determined.
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
    m_gpsInfoCache = make_unique<location::GpsInfo>(gpsInfo);

  auto routeMatchingInfo = GetRouteMatchingInfo(gpsInfo);
  m_drapeEngine.SafeCall(&df::DrapeEngine::SetGpsInfo, gpsInfo, m_routingSession.IsNavigable(),
                         routeMatchingInfo);

  if (IsTrackingReporterEnabled())
    m_trackingReporter.AddLocation(gpsInfo, m_routingSession.MatchTraffic(routeMatchingInfo));
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

m2::RectD RoutingManager::ShowPreviewSegments(vector<RouteMarkData> const & routePoints)
{
  df::DrapeEngineLockGuard lock(m_drapeEngine);
  if (!lock)
    return MercatorBounds::FullRect();

  m2::RectD rect;
  for (size_t pointIndex = 0; pointIndex + 1 < routePoints.size(); pointIndex++)
  {
    rect.Add(routePoints[pointIndex].m_position);
    rect.Add(routePoints[pointIndex + 1].m_position);
    lock.Get()->AddRoutePreviewSegment(routePoints[pointIndex].m_position,
                                       routePoints[pointIndex + 1].m_position);
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
    m_loadRoutePointsTimestamp = chrono::steady_clock::time_point();
}

TransitRouteInfo RoutingManager::GetTransitRouteInfo() const
{
  lock_guard<mutex> lock(m_drapeSubroutesMutex);
  return m_transitRouteInfo;
}

void RoutingManager::SetSubroutesVisibility(bool visible)
{
  df::DrapeEngineLockGuard lock(m_drapeEngine);
  if (!lock)
    return;

  lock_guard<mutex> lockSubroutes(m_drapeSubroutesMutex);
  for (auto const & subrouteId : m_drapeSubroutes)
    lock.Get()->SetSubrouteVisibility(subrouteId, visible);
}

bool RoutingManager::IsSpeedLimitExceeded() const
{
  return m_routingSession.IsSpeedLimitExceeded();
}
