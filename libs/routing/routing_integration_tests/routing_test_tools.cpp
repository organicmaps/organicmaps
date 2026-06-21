#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "testing/testing.hpp"

#include "map/features_fetcher.hpp"
#include "map/transit/transit_reader.hpp"

#include "drape_frontend/route_shape.hpp"

#include "routing/index_router.hpp"
#include "routing/route.hpp"
#include "routing/router_delegate.hpp"
#include "routing/routing_callbacks.hpp"

#include "storage/country_parent_getter.hpp"
#include "storage/routing_helpers.hpp"

#include "indexer/data_source.hpp"

#include "platform/platform_tests_support/helpers.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "base/math.hpp"
#include "base/stl_helpers.hpp"
#include "base/strings_bundle.hpp"

namespace integration
{
using namespace routing;
using namespace std;

namespace
{
double constexpr kErrorMeters = 1.0;
double constexpr kErrorSeconds = 1.0;
}  // namespace

shared_ptr<FeaturesFetcher> CreateFeaturesFetcher(vector<LocalCountryFile> const & localFiles)
{
  size_t const maxOpenFileNumber = 4096;
  platform::tests_support::ChangeMaxNumberOfOpenFiles(maxOpenFileNumber);
  shared_ptr<FeaturesFetcher> featuresFetcher(new FeaturesFetcher);
  featuresFetcher->InitClassificator();

  for (LocalCountryFile const & localFile : localFiles)
    featuresFetcher->RegisterMap(localFile);

  return featuresFetcher;
}

unique_ptr<storage::CountryInfoGetter> CreateCountryInfoGetter()
{
  Platform const & platform = GetPlatform();
  return storage::CountryInfoReader::CreateCountryInfoGetter(platform);
}

unique_ptr<IndexRouter> CreateVehicleRouter(DataSource & dataSource, storage::CountryInfoGetter const & infoGetter,
                                            traffic::TrafficCache const & trafficCache,
                                            vector<LocalCountryFile> const & localFiles, VehicleType vehicleType)
{
  auto const countryFileGetter = [&infoGetter](m2::PointD const & pt) { return infoGetter.GetRegionCountryId(pt); };

  auto const getMwmRectByName = [&infoGetter](string const & countryId) -> m2::RectD
  { return infoGetter.GetLimitRectForLeaf(countryId); };

  auto countryParentGetter = std::make_unique<storage::CountryParentGetter>();
  CHECK(countryParentGetter, ());

  auto numMwmIds = make_shared<NumMwmIds>();
  for (auto const & f : localFiles)
  {
    auto const & countryFile = f.GetCountryFile();
    auto const mwmId = dataSource.GetMwmIdByCountryFile(countryFile);

    if (!mwmId.IsAlive())
      continue;

    if (countryParentGetter->GetStorageForTesting().IsLeaf(countryFile.GetName()))
      numMwmIds->RegisterFile(countryFile);
  }

  // You should have at least one country file to make further tests.
  TEST(!numMwmIds->IsEmpty(), ());

  bool const loadAltitudes = vehicleType != VehicleType::Car;
  auto indexRouter =
      make_unique<IndexRouter>(vehicleType, loadAltitudes, *countryParentGetter, countryFileGetter, getMwmRectByName,
                               numMwmIds, MakeNumMwmTree(*numMwmIds, infoGetter), trafficCache, dataSource);

  return indexRouter;
}

void GetAllLocalFiles(vector<LocalCountryFile> & localFiles)
{
  platform::FindAllLocalMapsAndCleanup(numeric_limits<int64_t>::max() /* latestVersion */, localFiles);
  platform::FindAllLocalMapsInResourcesDir(localFiles);

  // Leave only real country files for routing test.
  localFiles.erase(std::remove_if(localFiles.begin(), localFiles.end(),
                                  [](LocalCountryFile const & file)
  {
    auto const & name = file.GetCountryName();
    return name == WORLD_FILE_NAME || name == WORLD_COASTS_FILE_NAME;
  }),
                   localFiles.end());

  for (auto & file : localFiles)
    file.SyncWithDisk();
}

shared_ptr<VehicleRouterComponents> CreateAllMapsComponents(VehicleType vehicleType,
                                                            std::set<std::string> const & skipMaps)
{
  vector<LocalCountryFile> localFiles;
  GetAllLocalFiles(localFiles);
  base::EraseIf(localFiles,
                [&skipMaps](LocalCountryFile const & cf) { return skipMaps.count(cf.GetCountryName()) > 0; });
  TEST(!localFiles.empty(), ());

  return make_shared<VehicleRouterComponents>(localFiles, vehicleType);
}

IRouterComponents & GetVehicleComponents(VehicleType vehicleType)
{
  static map<VehicleType, shared_ptr<VehicleRouterComponents>> kVehicleComponents;

  auto it = kVehicleComponents.find(vehicleType);
  if (it == kVehicleComponents.end())
    tie(it, ignore) = kVehicleComponents.emplace(vehicleType, CreateAllMapsComponents(vehicleType, {}));

  CHECK(it->second, ());
  return *(it->second);
}

namespace
{
shared_ptr<Route> PromoteActive(RoutesResult const & res)
{
  if (res.IsValid())
    return make_shared<Route>(res.GetActive());
  return {};
}
}  // namespace

TRouteResult CalculateRoute(IRouterComponents const & routerComponents, m2::PointD const & startPoint,
                            m2::PointD const & startDirection, m2::PointD const & finalPoint)
{
  RouterDelegate delegate;
  RoutesResult res("mapsme", 0 /* routes id */);
  RouterResultCode result = routerComponents.GetRouter().CalculateRoute(
      Checkpoints(startPoint, finalPoint), startDirection, false /* adjust */, delegate, res);
  routerComponents.GetRouter().SetGuides({});
  return TRouteResult(PromoteActive(res), result);
}

TRouteResult CalculateRoute(IRouterComponents const & routerComponents, Checkpoints const & checkpoints,
                            GuidesTracks && guides)
{
  RouterDelegate delegate;
  RoutesResult res("mapsme", 0 /* routes id */);
  routerComponents.GetRouter().SetGuides(std::move(guides));
  RouterResultCode result = routerComponents.GetRouter().CalculateRoute(
      checkpoints, m2::PointD::Zero() /* startDirection */, false /* adjust */, delegate, res);
  routerComponents.GetRouter().SetGuides({});
  return TRouteResult(PromoteActive(res), result);
}

TRoutesResult CalculateRoutes(IRouterComponents const & routerComponents, Checkpoints const & checkpoints)
{
  RouterDelegate delegate;
  RoutesResult res("mapsme", 0 /* routes id */);
  RouterResultCode const result = routerComponents.GetRouter().CalculateRoute(
      checkpoints, m2::PointD::Zero() /* startDirection */, false /* adjust */, delegate, res);
  routerComponents.GetRouter().SetGuides({});

  vector<shared_ptr<Route>> routes;
  if (res.IsValid())
  {
    // Active (primary) route first, then the remaining alternatives in their stored order.
    routes.push_back(make_shared<Route>(res.m_routes[res.m_activeIdx]));
    for (size_t i = 0; i < res.m_routes.size(); ++i)
      if (i != res.m_activeIdx)
        routes.push_back(make_shared<Route>(res.m_routes[i]));
  }
  return {std::move(routes), result};
}

TransitRouteInfo GetTransitRouteInfo(IRouterComponents const & routerComponents, Route const & route)
{
  auto & fetcher = routerComponents.GetFeaturesFetcher();
  auto & dataSource = fetcher.GetDataSource();

  TransitReadManager readManager(dataSource, [&fetcher](FeatureCallback const & fn, vector<FeatureID> const & ids)
  { fetcher.ReadFeatures(fn, ids); }, [](m2::RectD const &) { return vector<MwmSet::MwmId>(); });

  auto const & numMwmIds = static_cast<IndexRouter &>(routerComponents.GetRouter()).GetNumMwmIds();
  auto const getMwmIdFn = [&](NumMwmId numMwmId)
  { return dataSource.GetMwmIdByCountryFile(numMwmIds->GetFile(numMwmId)); };

  static StringsBundle const kEmptyBundle;
  static std::map<std::string, m2::PointF> const kNoSymbols;
  TransitRouteDisplay display(readManager, getMwmIdFn, []() -> StringsBundle const & { return kEmptyBundle; },
                              nullptr /* bmManager */, kNoSymbols);

  vector<RouteSegment> segments;
  for (size_t i = 0; i < route.GetSubrouteCount(); ++i)
  {
    route.GetSubrouteInfo(i, segments);
    df::Subroute subroute;
    // ProcessSubroute appends to the polyline and asserts it is non-empty; production seeds it with
    // the subroute start point in CreateDrapeSubroute, so do the same here.
    subroute.m_polyline.Add(route.GetSubrouteAttrs(i).GetStart().GetPoint());
    display.ProcessSubroute(segments, subroute);
  }

  return display.GetRouteInfo();
}

double GetWalkDistanceMeters(Route const & route)
{
  double walkMeters = 0.0;
  double prevMeters = 0.0;
  for (auto const & s : route.GetRouteSegments())
  {
    double const curMeters = s.GetDistFromBeginningMeters();
    if (!s.HasTransitInfo())
      walkMeters += curMeters - prevMeters;
    prevMeters = curMeters;
  }
  return walkMeters;
}

void TestTurnCount(routing::Route const & route, uint32_t expectedTurnCount)
{
  // We use -1 for ignoring the "ReachedYourDestination" turn record.
  vector<turns::TurnItem> turns;
  route.GetTurnsForTesting(turns);
  TEST_EQUAL(turns.size() - 1, expectedTurnCount, ());
}

void TestTurns(Route const & route, vector<CarDirection> const & expectedTurns)
{
  vector<turns::TurnItem> turns;
  route.GetTurnsForTesting(turns);
  TEST_EQUAL(turns.size() - 1, expectedTurns.size(), ());

  for (size_t i = 0; i < expectedTurns.size(); ++i)
    TEST_EQUAL(turns[i].m_turn, expectedTurns[i], ());
}

void TestCurrentStreetName(Route const & route, string const & expectedStreetName)
{
  RouteSegment::RoadNameInfo roadNameInfo;
  route.GetCurrentStreetName(roadNameInfo);
  TEST_EQUAL(roadNameInfo.m_name, expectedStreetName, ());
}

void TestNextStreetName(Route const & route, string const & expectedStreetName)
{
  RouteSegment::RoadNameInfo roadNameInfo;
  route.GetNextTurnStreetName(roadNameInfo);
  TEST_EQUAL(roadNameInfo.m_name, expectedStreetName, ());
}

void TestRouteLength(Route const & route, double expectedRouteMeters, double relativeError)
{
  double const delta = max(expectedRouteMeters * relativeError, kErrorMeters);
  double const routeMeters = route.GetTotalDistanceMeters();
  TEST(AlmostEqualAbs(routeMeters, expectedRouteMeters, delta),
       ("Route length test failed. Expected:", expectedRouteMeters, "have:", routeMeters, "delta:", delta));
}

void TestRouteTime(Route const & route, double expectedRouteSeconds, double relativeError)
{
  double const delta = max(expectedRouteSeconds * relativeError, kErrorSeconds);
  double const routeSeconds = route.GetTotalTimeSec();
  TEST(AlmostEqualAbs(routeSeconds, expectedRouteSeconds, delta),
       ("Route time test failed. Expected:", expectedRouteSeconds, "have:", routeSeconds, "delta:", delta));
}

void TestRoutePointsNumber(Route const & route, size_t expectedPointsNumber, double relativeError)
{
  CHECK_GREATER_OR_EQUAL(relativeError, 0.0, ());
  size_t const routePoints = route.GetPoly().GetSize();
  TEST(AlmostEqualRel(static_cast<double>(routePoints), static_cast<double>(expectedPointsNumber), relativeError),
       ("Route points test failed. Expected:", expectedPointsNumber, "have:", routePoints,
        "relative error:", relativeError));
}

void CalculateRouteAndTestRouteLength(IRouterComponents const & routerComponents, m2::PointD const & startPoint,
                                      m2::PointD const & startDirection, m2::PointD const & finalPoint,
                                      double expectedRouteMeters, double relativeError /* = 0.02 */)
{
  TRouteResult routeResult = CalculateRoute(routerComponents, startPoint, startDirection, finalPoint);
  RouterResultCode const result = routeResult.second;
  TEST_EQUAL(result, RouterResultCode::NoError, ());
  CHECK(routeResult.first, ());
  TestRouteLength(*routeResult.first, expectedRouteMeters, relativeError);
}

void CalculateRouteAndTestRouteTime(IRouterComponents const & routerComponents, m2::PointD const & startPoint,
                                    m2::PointD const & startDirection, m2::PointD const & finalPoint,
                                    double expectedTimeSeconds, double relativeError /* = 0.07 */)
{
  TRouteResult routeResult = CalculateRoute(routerComponents, startPoint, startDirection, finalPoint);
  RouterResultCode const result = routeResult.second;
  TEST_EQUAL(result, RouterResultCode::NoError, ());
  CHECK(routeResult.first, ());
  TestRouteTime(*routeResult.first, expectedTimeSeconds, relativeError);
}

TestTurn const & TestTurn::TestValid() const
{
  TEST(m_isValid, ());
  return *this;
}

TestTurn const & TestTurn::TestNotValid() const
{
  TEST(!m_isValid, ());
  return *this;
}

TestTurn const & TestTurn::TestPoint(m2::PointD const & expectedPoint, double inaccuracyMeters) const
{
  double const distanceMeters = ms::DistanceOnEarth(expectedPoint.y, expectedPoint.x, m_point.y, m_point.x);
  TEST_LESS(distanceMeters, inaccuracyMeters, ());
  return *this;
}

TestTurn const & TestTurn::TestDirection(routing::turns::CarDirection expectedDirection) const
{
  TEST_EQUAL(m_direction, expectedDirection, ());
  return *this;
}

TestTurn const & TestTurn::TestOneOfDirections(set<routing::turns::CarDirection> const & expectedDirections) const
{
  TEST(expectedDirections.find(m_direction) != expectedDirections.cend(), (m_direction));
  return *this;
}

TestTurn const & TestTurn::TestRoundAboutExitNum(uint32_t expectedRoundAboutExitNum) const
{
  TEST_EQUAL(m_roundAboutExitNum, expectedRoundAboutExitNum, ());
  return *this;
}

TestTurn GetNthTurn(routing::Route const & route, uint32_t turnNumber)
{
  vector<turns::TurnItem> turns;
  route.GetTurnsForTesting(turns);
  if (turnNumber >= turns.size())
  {
    ASSERT(false, ());
    return TestTurn();
  }

  TurnItem const & turn = turns[turnNumber];
  return TestTurn(route.GetPoly().GetPoint(turn.m_index), turn.m_turn, turn.m_exitNum);
}

bool IsSubwayExists(Route const & route)
{
  auto const & routeSegments = route.GetRouteSegments();
  bool intoSubway = false;

  for (auto const & routeSegment : routeSegments)
  {
    if (!routeSegment.HasTransitInfo())
    {
      intoSubway = false;
      continue;
    }

    if (routeSegment.GetTransitInfo().GetType() != TransitInfo::Type::Gate)
      continue;

    if (intoSubway)
      return true;

    intoSubway = true;
  }

  return false;
}

void CheckSubwayAbsent(Route const & route)
{
  bool wasSubway = IsSubwayExists(route);
  TEST(!wasSubway, ("Find subway subpath into route."));
}

void CheckSubwayExistence(Route const & route)
{
  bool wasSubway = IsSubwayExists(route);
  TEST(wasSubway, ("Can not find subway subpath into route."));
}

LocalCountryFile GetLocalCountryFileByCountryId(platform::CountryFile const & country)
{
  vector<LocalCountryFile> localFiles;
  GetAllLocalFiles(localFiles);

  for (auto const & lf : localFiles)
    if (lf.GetCountryFile() == country)
      return lf;
  return {};
}
}  // namespace integration
