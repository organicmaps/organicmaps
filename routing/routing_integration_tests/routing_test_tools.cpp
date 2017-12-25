#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "routing/routing_tests/index_graph_tools.hpp"

#include "testing/testing.hpp"

#include "map/feature_vec_model.hpp"

#include "storage/routing_helpers.hpp"

#include "routing/index_router.hpp"
#include "routing/online_absent_fetcher.hpp"
#include "routing/online_cross_fetcher.hpp"
#include "routing/road_graph_router.hpp"
#include "routing/route.hpp"
#include "routing/router_delegate.hpp"

#include "indexer/index.hpp"

#include "storage/country_parent_getter.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/latlon.hpp"

#include "base/math.hpp"
#include "base/stl_add.hpp"

#include "std/functional.hpp"
#include "std/limits.hpp"

#include "private.h"

#include <sys/resource.h>

using namespace routing;
using namespace routing_test;

using TRouterFactory =
    function<unique_ptr<IRouter>(Index & index, TCountryFileFn const & countryFileFn,
                                 shared_ptr<NumMwmIds> numMwmIds)>;

namespace
{
double constexpr kErrorMeters = 1.0;
double constexpr kErrorSeconds = 1.0;

void ChangeMaxNumberOfOpenFiles(size_t n)
{
  struct rlimit rlp;
  getrlimit(RLIMIT_NOFILE, &rlp);
  rlp.rlim_cur = n;
  setrlimit(RLIMIT_NOFILE, &rlp);
}
}  // namespace

namespace integration
{
  shared_ptr<model::FeaturesFetcher> CreateFeaturesFetcher(vector<LocalCountryFile> const & localFiles)
  {
    size_t const maxOpenFileNumber = 4096;
    ChangeMaxNumberOfOpenFiles(maxOpenFileNumber);
    shared_ptr<model::FeaturesFetcher> featuresFetcher(new model::FeaturesFetcher);
    featuresFetcher->InitClassificator();

    for (LocalCountryFile const & localFile : localFiles)
    {
      auto p = featuresFetcher->RegisterMap(localFile);
      if (p.second != MwmSet::RegResult::Success)
      {
        ASSERT(false, ("Can't register", localFile));
        return nullptr;
      }
    }
    return featuresFetcher;
  }

  unique_ptr<storage::CountryInfoGetter> CreateCountryInfoGetter()
  {
    Platform const & platform = GetPlatform();
    return storage::CountryInfoReader::CreateCountryInfoReader(platform);
  }

  unique_ptr<IndexRouter> CreateVehicleRouter(Index & index,
                                              storage::CountryInfoGetter const & infoGetter,
                                              traffic::TrafficCache const & trafficCache,
                                              vector<LocalCountryFile> const & localFiles,
                                              VehicleType vehicleType)
  {
    auto const countryFileGetter = [&infoGetter](m2::PointD const & pt) {
      return infoGetter.GetRegionCountryId(pt);
    };

    auto const getMwmRectByName = [&infoGetter](string const & countryId) -> m2::RectD {
      return infoGetter.GetLimitRectForLeaf(countryId);
    };

    auto numMwmIds = make_shared<NumMwmIds>();
    for (auto const & f : localFiles)
    {
      auto const & countryFile = f.GetCountryFile();
      auto const mwmId = index.GetMwmIdByCountryFile(countryFile);
      if (mwmId.GetInfo()->GetType() == MwmInfo::COUNTRY && countryFile.GetName() != "minsk-pass")
        numMwmIds->RegisterFile(countryFile);
    }

    auto countryParentGetter = my::make_unique<storage::CountryParentGetter>();
    CHECK(countryParentGetter, ());

    auto indexRouter = make_unique<IndexRouter>(vehicleType, false /* load altitudes*/,
                                                *countryParentGetter, countryFileGetter,
                                                getMwmRectByName, numMwmIds,
                                                MakeNumMwmTree(*numMwmIds, infoGetter), trafficCache, index);

    return indexRouter;
  }

  unique_ptr<IRouter> CreateAStarRouter(Index & index,
                                        storage::CountryInfoGetter const & infoGetter,
                                        vector<LocalCountryFile> const & localFiles,
                                        TRouterFactory const & routerFactory)
  {
    // |infoGetter| should be a reference to an object which exists while the
    // result of the function is used.
    auto countryFileGetter = [&infoGetter](m2::PointD const & pt)
    {
      return infoGetter.GetRegionCountryId(pt);
    };

    auto numMwmIds = make_shared<NumMwmIds>();
    for (auto const & file : localFiles)
      numMwmIds->RegisterFile(file.GetCountryFile());

    unique_ptr<IRouter> router = routerFactory(index, countryFileGetter, numMwmIds);
    return unique_ptr<IRouter>(move(router));
  }

  void GetAllLocalFiles(vector<LocalCountryFile> & localFiles)
  {
    // Setting stored paths from testingmain.cpp
    Platform & pl = GetPlatform();
    CommandLineOptions const & options = GetTestingOptions();
    if (options.m_dataPath)
      pl.SetWritableDirForTests(options.m_dataPath);
    if (options.m_resourcePath)
      pl.SetResourceDir(options.m_resourcePath);

    platform::migrate::SetMigrationFlag();
    platform::FindAllLocalMapsAndCleanup(numeric_limits<int64_t>::max() /* latestVersion */,
                                         localFiles);
    for (auto & file : localFiles)
      file.SyncWithDisk();
  }

  shared_ptr<VehicleRouterComponents> CreateAllMapsComponents(VehicleType vehicleType)
  {
    vector<LocalCountryFile> localFiles;
    GetAllLocalFiles(localFiles);
    ASSERT(!localFiles.empty(), ());
    return make_shared<VehicleRouterComponents>(localFiles, vehicleType);
  }

  TRouteResult CalculateRoute(IRouterComponents const & routerComponents,
                              m2::PointD const & startPoint, m2::PointD const & startDirection,
                              m2::PointD const & finalPoint)
  {
    RouterDelegate delegate;
    shared_ptr<Route> route(new Route("mapsme"));
    IRouter::ResultCode result = routerComponents.GetRouter().CalculateRoute(
        Checkpoints(startPoint, finalPoint), startDirection, false /* adjust */, delegate, *route);
    ASSERT(route, ());
    return TRouteResult(route, result);
  }

  void TestTurnCount(routing::Route const & route, uint32_t expectedTurnCount)
  {
    // We use -1 for ignoring the "ReachedYourDestination" turn record.
    vector<turns::TurnItem> turns;
    route.GetTurnsForTesting(turns);
    TEST_EQUAL(turns.size() - 1, expectedTurnCount, ());
  }

  void TestCurrentStreetName(routing::Route const & route, string const & expectedStreetName)
  {
    string streetName;
    route.GetCurrentStreetName(streetName);
    TEST_EQUAL(streetName, expectedStreetName, ());
  }

  void TestNextStreetName(routing::Route const & route, string const & expectedStreetName)
  {
    string streetName;
    double distance;
    turns::TurnItem turn;
    route.GetCurrentTurn(distance, turn);
    route.GetStreetNameAfterIdx(turn.m_index, streetName);
    TEST_EQUAL(streetName, expectedStreetName, ());
  }

  void TestRouteLength(Route const & route, double expectedRouteMeters,
                       double relativeError)
  {
    double const delta = max(expectedRouteMeters * relativeError, kErrorMeters);
    double const routeMeters = route.GetTotalDistanceMeters();
    TEST(my::AlmostEqualAbs(routeMeters, expectedRouteMeters, delta),
        ("Route length test failed. Expected:", expectedRouteMeters, "have:", routeMeters, "delta:", delta));
  }

  void TestRouteTime(Route const & route, double expectedRouteSeconds, double relativeError)
  {
    double const delta = max(expectedRouteSeconds * relativeError, kErrorSeconds);
    double const routeSeconds = route.GetTotalTimeSec();
    TEST(my::AlmostEqualAbs(routeSeconds, expectedRouteSeconds, delta),
        ("Route time test failed. Expected:", expectedRouteSeconds, "have:", routeSeconds, "delta:", delta));
  }

  void TestRoutePointsNumber(Route const & route, size_t expectedPointsNumber, double relativeError)
  {
    CHECK_GREATER_OR_EQUAL(relativeError, 0.0, ());
    size_t const routePoints = route.GetPoly().GetSize();
    TEST(my::AlmostEqualRel(static_cast<double>(routePoints),
                            static_cast<double>(expectedPointsNumber), relativeError),
         ("Route points test failed. Expected:", expectedPointsNumber, "have:", routePoints,
          "relative error:", relativeError));
  }

  void CalculateRouteAndTestRouteLength(IRouterComponents const & routerComponents,
                                        m2::PointD const & startPoint,
                                        m2::PointD const & startDirection,
                                        m2::PointD const & finalPoint, double expectedRouteMeters,
                                        double relativeError)
  {
    TRouteResult routeResult =
        CalculateRoute(routerComponents, startPoint, startDirection, finalPoint);
    IRouter::ResultCode const result = routeResult.second;
    TEST_EQUAL(result, IRouter::NoError, ());
    TestRouteLength(*routeResult.first, expectedRouteMeters, relativeError);
  }

  const TestTurn & TestTurn::TestValid() const
  {
    TEST(m_isValid, ());
    return *this;
  }

  const TestTurn & TestTurn::TestNotValid() const
  {
    TEST(!m_isValid, ());
    return *this;
  }

  const TestTurn & TestTurn::TestPoint(m2::PointD const & expectedPoint, double inaccuracyMeters) const
  {
    double const distanceMeters = ms::DistanceOnEarth(expectedPoint.y, expectedPoint.x, m_point.y, m_point.x);
    TEST_LESS(distanceMeters, inaccuracyMeters, ());
    return *this;
  }

  const TestTurn & TestTurn::TestDirection(routing::turns::CarDirection expectedDirection) const
  {
    TEST_EQUAL(m_direction, expectedDirection, ());
    return *this;
  }

  const TestTurn & TestTurn::TestOneOfDirections(
      set<routing::turns::CarDirection> const & expectedDirections) const
  {
    TEST(expectedDirections.find(m_direction) != expectedDirections.cend(), ());
    return *this;
  }

  const TestTurn & TestTurn::TestRoundAboutExitNum(uint32_t expectedRoundAboutExitNum) const
  {
    TEST_EQUAL(m_roundAboutExitNum, expectedRoundAboutExitNum, ());
    return *this;
  }

  TestTurn GetNthTurn(routing::Route const & route, uint32_t turnNumber)
  {
    vector<turns::TurnItem> turns;
    route.GetTurnsForTesting(turns);
    if (turnNumber >= turns.size())
      return TestTurn();

    TurnItem const & turn = turns[turnNumber];
    return TestTurn(route.GetPoly().GetPoint(turn.m_index), turn.m_turn, turn.m_exitNum);
  }

  void TestOnlineFetcher(ms::LatLon const & startPoint, ms::LatLon const & finalPoint,
                         vector<string> const & expected, IRouterComponents & routerComponents)
  {
    auto countryFileGetter = [&routerComponents](m2::PointD const & p) -> string
    {
      return routerComponents.GetCountryInfoGetter().GetRegionCountryId(p);
    };
    auto localFileChecker =
        [&routerComponents](string const & /* countryFile */) -> bool
    {
      // Always returns that the file is absent.
      return false;
    };
    routing::OnlineAbsentCountriesFetcher fetcher(countryFileGetter, localFileChecker);
    fetcher.GenerateRequest(Checkpoints(MercatorBounds::FromLatLon(startPoint),
                                        MercatorBounds::FromLatLon(finalPoint)));
    vector<string> absent;
    fetcher.GetAbsentCountries(absent);
    if (expected.size() < 2)
    {
      // Single MWM case. Do not use online routing.
      TEST(absent.empty(), ());
      return;
    }
    TEST_EQUAL(absent.size(), expected.size(), ());
    for (string const & name : expected)
      TEST(find(absent.begin(), absent.end(), name) != absent.end(), ("Can't find", name));
  }

  void TestOnlineCrosses(ms::LatLon const & startPoint, ms::LatLon const & finalPoint,
                         vector<string> const & expected,
                         IRouterComponents & routerComponents)
  {
    TCountryFileFn const countryFileGetter = [&](m2::PointD const & p) {
      return routerComponents.GetCountryInfoGetter().GetRegionCountryId(p);
    };
    routing::OnlineCrossFetcher fetcher(countryFileGetter, OSRM_ONLINE_SERVER_URL,
                                        Checkpoints(MercatorBounds::FromLatLon(startPoint),
                                                    MercatorBounds::FromLatLon(finalPoint)));
    fetcher.Do();
    vector<m2::PointD> const & points = fetcher.GetMwmPoints();
    set<string> foundMwms;

    for (m2::PointD const & point : points)
    {
      string const mwmName = routerComponents.GetCountryInfoGetter().GetRegionCountryId(point);
      TEST(find(expected.begin(), expected.end(), mwmName) != expected.end(),
           ("Can't find ", mwmName));
      foundMwms.insert(mwmName);
    }
    TEST_EQUAL(expected.size(), foundMwms.size(), ());
  }
}
