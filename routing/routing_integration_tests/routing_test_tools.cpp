#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "testing/testing.hpp"

#include "map/feature_vec_model.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/latlon.hpp"

#include "routing/online_absent_fetcher.hpp"
#include "routing/online_cross_fetcher.hpp"
#include "routing/road_graph_router.hpp"
#include "routing/route.hpp"
#include "routing/router_delegate.hpp"

#include "indexer/index.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "std/limits.hpp"

#include "private.h"

#include <sys/resource.h>


using namespace routing;

using TRouterFactory =
    function<unique_ptr<IRouter>(Index & index, TCountryFileFn const & countryFileFn)>;

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
    size_t const maxOpenFileNumber = 1024;
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

  unique_ptr<CarRouter> CreateCarRouter(Index & index,
                                        storage::CountryInfoGetter const & infoGetter)
  {
    auto const countryFileGetter = [&infoGetter](m2::PointD const & pt)
    {
      return infoGetter.GetRegionCountryId(pt);
    };
    unique_ptr<CarRouter> carRouter(new CarRouter(&index, countryFileGetter,
                                    CreateCarAStarBidirectionalRouter(index, countryFileGetter)));
    return carRouter;
  }

  unique_ptr<IRouter> CreateAStarRouter(Index & index,
                                        storage::CountryInfoGetter const & infoGetter,
                                        TRouterFactory const & routerFactory)
  {
    // |infoGetter| should be a reference to an object which exists while the
    // result of the function is used.
    auto countryFileGetter = [&infoGetter](m2::PointD const & pt)
    {
      return infoGetter.GetRegionCountryId(pt);
    };
    unique_ptr<IRouter> router = routerFactory(index, countryFileGetter);
    return unique_ptr<IRouter>(move(router));
  }

  class OsrmRouterComponents : public IRouterComponents
  {
  public:
    OsrmRouterComponents(vector<LocalCountryFile> const & localFiles)
      : IRouterComponents(localFiles)
      , m_carRouter(CreateCarRouter(m_featuresFetcher->GetIndex(), *m_infoGetter))
    {
    }

    IRouter * GetRouter() const override { return m_carRouter.get(); }

  private:
    unique_ptr<CarRouter> m_carRouter;
  };

  class PedestrianRouterComponents : public IRouterComponents
  {
  public:
    PedestrianRouterComponents(vector<LocalCountryFile> const & localFiles)
      : IRouterComponents(localFiles)
      , m_router(CreateAStarRouter(m_featuresFetcher->GetIndex(), *m_infoGetter,
                                   CreatePedestrianAStarBidirectionalRouter))
    {
    }

    IRouter * GetRouter() const override { return m_router.get(); }

  private:
    unique_ptr<IRouter> m_router;
  };

  class BicycleRouterComponents : public IRouterComponents
  {
  public:
    BicycleRouterComponents(vector<LocalCountryFile> const & localFiles)
      : IRouterComponents(localFiles)
      , m_router(CreateAStarRouter(m_featuresFetcher->GetIndex(), *m_infoGetter,
                                   CreateBicycleAStarBidirectionalRouter))
    {
    }

    IRouter * GetRouter() const override { return m_router.get(); }

  private:
    unique_ptr<IRouter> m_router;
  };

  template <typename TRouterComponents>
  shared_ptr<TRouterComponents> CreateAllMapsComponents()
  {
    // Setting stored paths from testingmain.cpp
    Platform & pl = GetPlatform();
    CommandLineOptions const & options = GetTestingOptions();
    if (options.m_dataPath)
      pl.SetWritableDirForTests(options.m_dataPath);
    if (options.m_resourcePath)
      pl.SetResourceDir(options.m_resourcePath);

    platform::migrate::SetMigrationFlag();

    vector<LocalCountryFile> localFiles;
    platform::FindAllLocalMapsAndCleanup(numeric_limits<int64_t>::max() /* latestVersion */,
                                         localFiles);
    for (auto & file : localFiles)
      file.SyncWithDisk();
    ASSERT(!localFiles.empty(), ());
    return shared_ptr<TRouterComponents>(new TRouterComponents(localFiles));
  }

  shared_ptr<IRouterComponents> GetOsrmComponents(vector<platform::LocalCountryFile> const & localFiles)
  {
    return shared_ptr<IRouterComponents>(new OsrmRouterComponents(localFiles));
  }

  IRouterComponents & GetOsrmComponents()
  {
    static auto const instance = CreateAllMapsComponents<OsrmRouterComponents>();
    ASSERT(instance, ());
    return *instance;
  }

  shared_ptr<IRouterComponents> GetPedestrianComponents(vector<platform::LocalCountryFile> const & localFiles)
  {
    return make_shared<PedestrianRouterComponents>(localFiles);
  }

  IRouterComponents & GetPedestrianComponents()
  {
    static auto const instance = CreateAllMapsComponents<PedestrianRouterComponents>();
    ASSERT(instance, ());
    return *instance;
  }

  shared_ptr<IRouterComponents> GetBicycleComponents(
      vector<platform::LocalCountryFile> const & localFiles)
  {
    return make_shared<BicycleRouterComponents>(localFiles);
  }

  IRouterComponents & GetBicycleComponents()
  {
    static auto const instance = CreateAllMapsComponents<BicycleRouterComponents>();
    ASSERT(instance, ());
    return *instance;
  }

  TRouteResult CalculateRoute(IRouterComponents const & routerComponents,
                              m2::PointD const & startPoint, m2::PointD const & startDirection,
                              m2::PointD const & finalPoint)
  {
    RouterDelegate delegate;
    IRouter * router = routerComponents.GetRouter();
    ASSERT(router, ());
    shared_ptr<Route> route(new Route("mapsme"));
    IRouter::ResultCode result =
        router->CalculateRoute(startPoint, startDirection, finalPoint, delegate, *route.get());
    ASSERT(route, ());
    return TRouteResult(route, result);
  }

  void TestTurnCount(routing::Route const & route, uint32_t expectedTurnCount)
  {
    // We use -1 for ignoring the "ReachedYourDestination" turn record.
    TEST_EQUAL(route.GetTurns().size() - 1, expectedTurnCount, ());
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
    TEST(route.GetCurrentTurn(distance, turn), ());
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

  const TestTurn & TestTurn::TestDirection(routing::turns::TurnDirection expectedDirection) const
  {
    TEST_EQUAL(m_direction, expectedDirection, ());
    return *this;
  }

  const TestTurn & TestTurn::TestOneOfDirections(
      set<routing::turns::TurnDirection> const & expectedDirections) const
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
    Route::TTurns const & turns = route.GetTurns();
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
    fetcher.GenerateRequest(MercatorBounds::FromLatLon(startPoint),
                            MercatorBounds::FromLatLon(finalPoint));
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
    routing::OnlineCrossFetcher fetcher(OSRM_ONLINE_SERVER_URL, startPoint, finalPoint);
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
