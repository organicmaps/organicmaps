#include "routing_manager.hpp"

#include "map/chart_generator.hpp"
#include "map/mwm_tree.hpp"

#include "tracking/reporter.hpp"

#include "routing/car_router.hpp"
#include "routing/index_router.hpp"
#include "routing/num_mwm_id.hpp"
#include "routing/online_absent_fetcher.hpp"
#include "routing/road_graph_router.hpp"
#include "routing/route.hpp"
#include "routing/routing_algorithm.hpp"
#include "routing/routing_helpers.hpp"

#include "drape_frontend/drape_engine.hpp"

#include "indexer/map_style_reader.hpp"
#include "indexer/scales.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"

#include "platform/country_file.hpp"
#include "platform/mwm_traits.hpp"
#include "platform/platform.hpp"

#include "3party/Alohalytics/src/alohalytics.h"

namespace
{
char const kRouterTypeKey[] = "router";
}  // namespace

namespace marketing
{
char const * const kRoutingCalculatingRoute = "Routing_CalculatingRoute";
}  // namespace marketing

using namespace routing;

RoutingManager::RoutingManager(Callbacks && callbacks, Delegate & delegate)
  : m_callbacks(std::move(callbacks)), m_delegate(delegate)
{
  auto const routingStatisticsFn = [](map<string, string> const & statistics) {
    alohalytics::LogEvent("Routing_CalculatingRoute", statistics);
    GetPlatform().GetMarketingService().SendMarketingEvent(marketing::kRoutingCalculatingRoute, {});
  };

  m_routingSession.Init(routingStatisticsFn, m_callbacks.m_visualizer);
  m_routingSession.SetReadyCallbacks(
      [&](Route const & route, IRouter::ResultCode code) { OnBuildRouteReady(route, code); },
      [&](Route const & route, IRouter::ResultCode code) { OnRebuildRouteReady(route, code); });
}

void RoutingManager::OnBuildRouteReady(Route const & route, IRouter::ResultCode code)
{
  storage::TCountriesVec absentCountries;
  if (code == IRouter::NoError)
  {
    double const kRouteScaleMultiplier = 1.5;

    InsertRoute(route);
    m_drapeEngine->StopLocationFollow();

    // Validate route (in case of bicycle routing it can be invalid).
    ASSERT(route.IsValid(), ());
    if (route.IsValid())
    {
      m2::RectD routeRect = route.GetPoly().GetLimitRect();
      routeRect.Scale(kRouteScaleMultiplier);
      m_drapeEngine->SetModelViewRect(routeRect, true /* applyRotation */, -1 /* zoom */,
                                      true /* isAnim */);
    }
  }
  else
  {
    absentCountries.assign(route.GetAbsentCountries().begin(), route.GetAbsentCountries().end());

    if (code != IRouter::NeedMoreMaps)
      RemoveRoute(true /* deactivateFollowing */);
  }
  CallRouteBuilded(code, absentCountries);
}

void RoutingManager::OnRebuildRouteReady(Route const & route, IRouter::ResultCode code)
{
  if (code != IRouter::NoError)
    return;

  RemoveRoute(false /* deactivateFollowing */);
  InsertRoute(route);
  CallRouteBuilded(code, storage::TCountriesVec());
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

  auto const routerType = routing::FromString(routerTypeStr);

  switch (routerType)
  {
  case RouterType::Pedestrian:
  case RouterType::Bicycle: return routerType;
  default: return RouterType::Vehicle;
  }
}

void RoutingManager::SetRouterImpl(routing::RouterType type)
{
  auto const indexGetterFn = m_callbacks.m_featureIndexGetter;
  ASSERT(indexGetterFn, ());
  unique_ptr<IRouter> router;
  unique_ptr<OnlineAbsentCountriesFetcher> fetcher;

  auto const countryFileGetter = [this](m2::PointD const & p) -> string {
    // TODO (@gorshenin): fix CountryInfoGetter to return CountryFile
    // instances instead of plain strings.
    return m_callbacks.m_countryInfoGetter().GetRegionCountryId(p);
  };

  if (type == RouterType::Pedestrian)
  {
    router = CreatePedestrianAStarBidirectionalRouter(indexGetterFn(), countryFileGetter);
    m_routingSession.SetRoutingSettings(routing::GetPedestrianRoutingSettings());
  }
  else if (type == RouterType::Bicycle)
  {
    router = CreateBicycleAStarBidirectionalRouter(indexGetterFn(), countryFileGetter);
    m_routingSession.SetRoutingSettings(routing::GetBicycleRoutingSettings());
  }
  else
  {
    auto & index = m_callbacks.m_featureIndexGetter();

    auto localFileChecker = [this](string const & countryFile) -> bool {
      MwmSet::MwmId const mwmId = m_callbacks.m_featureIndexGetter().GetMwmIdByCountryFile(
          platform::CountryFile(countryFile));
      if (!mwmId.IsAlive())
        return false;

      return version::MwmTraits(mwmId.GetInfo()->m_version).HasRoutingIndex();
    };

    auto numMwmIds = make_shared<routing::NumMwmIds>();

    m_delegate.RegisterCountryFiles(numMwmIds);

    auto const getMwmRectByName = [this](string const & countryId) -> m2::RectD {
      return m_callbacks.m_countryInfoGetter().GetLimitRectForLeaf(countryId);
    };

    router.reset(new CarRouter(
        index, countryFileGetter,
        IndexRouter::CreateCarRouter(countryFileGetter, getMwmRectByName, numMwmIds,
                                     MakeNumMwmTree(*numMwmIds, m_callbacks.m_countryInfoGetter()),
                                     m_routingSession, index)));
    fetcher.reset(new OnlineAbsentCountriesFetcher(countryFileGetter, localFileChecker));
    m_routingSession.SetRoutingSettings(routing::GetCarRoutingSettings());
  }

  m_routingSession.SetRouter(move(router), move(fetcher));
  m_currentRouterType = type;
}

void RoutingManager::RemoveRoute(bool deactivateFollowing)
{
  if (m_drapeEngine != nullptr)
    m_drapeEngine->RemoveRoute(deactivateFollowing);
}

void RoutingManager::InsertRoute(routing::Route const & route)
{
  if (m_drapeEngine == nullptr)
    return;

  if (route.GetPoly().GetSize() < 2)
  {
    LOG(LWARNING, ("Invalid track - only", route.GetPoly().GetSize(), "point(s)."));
    return;
  }

  vector<double> turns;
  if (m_currentRouterType == RouterType::Vehicle || m_currentRouterType == RouterType::Bicycle ||
      m_currentRouterType == RouterType::Taxi)
  {
    route.GetTurnsDistances(turns);
  }

  df::ColorConstant routeColor = df::kRouteColor;
  df::RoutePattern pattern;
  if (m_currentRouterType == RouterType::Pedestrian)
  {
    routeColor = df::kRoutePedestrian;
    pattern = df::RoutePattern(4.0, 2.0);
  }
  else if (m_currentRouterType == RouterType::Bicycle)
  {
    routeColor = df::kRouteBicycle;
    pattern = df::RoutePattern(8.0, 2.0);
  }

  m_drapeEngine->AddRoute(route.GetPoly(), turns, routeColor, route.GetTraffic(), pattern);
}

void RoutingManager::FollowRoute()
{
  ASSERT(m_drapeEngine != nullptr, ());

  if (!m_routingSession.EnableFollowMode())
    return;

  m_delegate.OnRouteFollow(m_currentRouterType);
  m_drapeEngine->SetRoutePoint(m2::PointD(), true /* isStart */, false /* isValid */);
}

void RoutingManager::CloseRouting()
{
  if (m_routingSession.IsBuilt())
  {
    m_routingSession.EmitCloseRoutingEvent();
  }
  m_routingSession.Reset();
  RemoveRoute(true /* deactivateFollowing */);
}

void RoutingManager::SetLastUsedRouter(RouterType type)
{
  settings::Set(kRouterTypeKey, routing::ToString(type));
}

void RoutingManager::SetRouteStartPoint(m2::PointD const & pt, bool isValid)
{
  if (m_drapeEngine != nullptr)
    m_drapeEngine->SetRoutePoint(pt, true /* isStart */, isValid);
}

void RoutingManager::SetRouteFinishPoint(m2::PointD const & pt, bool isValid)
{
  if (m_drapeEngine != nullptr)
    m_drapeEngine->SetRoutePoint(pt, false /* isStart */, isValid);
}

void RoutingManager::GenerateTurnNotifications(vector<string> & turnNotifications)
{
  if (m_currentRouterType == routing::RouterType::Taxi)
    return;

  return m_routingSession.GenerateTurnNotifications(turnNotifications);
}

void RoutingManager::BuildRoute(m2::PointD const & finish, uint32_t timeoutSec)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ("BuildRoute"));
  ASSERT(m_drapeEngine != nullptr, ());

  m2::PointD start;
  if (!m_drapeEngine->GetMyPosition(start))
  {
    CallRouteBuilded(IRouter::NoCurrentPosition, storage::TCountriesVec());
    return;
  }
  BuildRoute(start, finish, false /* isP2P */, timeoutSec);
}

void RoutingManager::BuildRoute(m2::PointD const & start, m2::PointD const & finish, bool isP2P,
                                uint32_t timeoutSec)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ("BuildRoute"));
  ASSERT(m_drapeEngine != nullptr, ());

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
    case RouterType::Count: CHECK(false, ("Bad router type", m_currentRouterType));
    }
    GetPlatform().GetMarketingService().SendPushWooshTag(tag);
  }

  if (IsRoutingActive())
    CloseRouting();

  m_routingSession.SetUserCurrentPosition(start);
  m_routingSession.BuildRoute(start, finish, timeoutSec);
}

bool RoutingManager::DisableFollowMode()
{
  bool const disabled = m_routingSession.DisableFollowMode();
  if (disabled && m_drapeEngine != nullptr)
    m_drapeEngine->DeactivateRouteFollowing();

  return disabled;
}

void RoutingManager::CheckLocationForRouting(location::GpsInfo const & info)
{
  if (!IsRoutingActive())
    return;

  auto const featureIndexGetterFn = m_callbacks.m_featureIndexGetter;
  ASSERT(featureIndexGetterFn, ());
  RoutingSession::State const state =
      m_routingSession.OnLocationPositionChanged(info, featureIndexGetterFn());
  if (state == RoutingSession::RouteNeedRebuild)
  {
    m_routingSession.RebuildRoute(
        MercatorBounds::FromLatLon(info.m_latitude, info.m_longitude),
        [&](Route const & route, IRouter::ResultCode code) { OnRebuildRouteReady(route, code); },
        0 /* timeoutSec */, routing::RoutingSession::State::RouteRebuilding);
  }
}

void RoutingManager::CallRouteBuilded(routing::IRouter::ResultCode code,
                                      storage::TCountriesVec const & absentCountries)
{
  if (code == IRouter::Cancelled)
    return;

  m_routingCallback(code, absentCountries);
}

void RoutingManager::MatchLocationToRoute(location::GpsInfo & location,
                                          location::RouteMatchingInfo & routeMatchingInfo) const
{
  if (!IsRoutingActive())
    return;

  m_routingSession.MatchLocationToRoute(location, routeMatchingInfo);
}

bool RoutingManager::HasRouteAltitude() const { return m_routingSession.HasRouteAltitude(); }
bool RoutingManager::GenerateRouteAltitudeChart(uint32_t width, uint32_t height,
                                                vector<uint8_t> & imageRGBAData,
                                                int32_t & minRouteAltitude,
                                                int32_t & maxRouteAltitude,
                                                measurement_utils::Units & altitudeUnits) const
{
  feature::TAltitudes altitudes;
  vector<double> segDistance;

  if (!m_routingSession.GetRouteAltitudesAndDistancesM(segDistance, altitudes))
    return false;
  segDistance.insert(segDistance.begin(), 0.0);

  if (altitudes.empty())
    return false;

  if (!maps::GenerateChart(width, height, segDistance, altitudes,
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
  if (m_currentRouterType != routing::RouterType::Vehicle)
    return false;

  if (!m_routingSession.IsFollowing())
    return false;

  bool enableTracking = false;
  UNUSED_VALUE(settings::Get(tracking::Reporter::kEnableTrackingKey, enableTracking));
  return enableTracking;
}

void RoutingManager::SetRouter(RouterType type)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ("SetRouter"));

  if (m_currentRouterType == type)
    return;

  SetLastUsedRouter(type);
  SetRouterImpl(type);
}
