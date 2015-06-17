#include "integration_tests/osrm_test_tools.hpp"

#include "testing/testing.hpp"

#include "indexer/index.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "routing/online_cross_fetcher.hpp"
#include "routing/route.hpp"

#include "map/feature_vec_model.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "search/search_engine.hpp"

#include <sys/resource.h>


using namespace routing;
using platform::LocalCountryFile;

namespace
{
  void ChangeMaxNumberOfOpenFiles(size_t n)
  {
    struct rlimit rlp;
    getrlimit(RLIMIT_NOFILE, &rlp);
    rlp.rlim_cur = n;
    setrlimit(RLIMIT_NOFILE, &rlp);
  }
}

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
      pair<MwmSet::MwmLock, bool> result = featuresFetcher->RegisterMap(localFile);
      if (!result.second)
      {
        ASSERT(false, ("Can't register", localFile));
        return nullptr;
      }
    }
    return featuresFetcher;
  }

  shared_ptr<search::Engine> CreateSearchEngine(shared_ptr<model::FeaturesFetcher> featuresFetcher)
  {
    ASSERT(featuresFetcher, ());
    search::Engine::IndexType const & index = featuresFetcher->GetIndex();

    Platform const & pl = GetPlatform();
    try
    {
      shared_ptr<search::Engine> searchEngine(new search::Engine(
                         &index,
                         pl.GetReader(SEARCH_CATEGORIES_FILE_NAME),
                         pl.GetReader(PACKED_POLYGONS_FILE),
                         pl.GetReader(COUNTRIES_FILE),
                         languages::GetCurrentOrig()));
      return searchEngine;
    }
    catch (RootException const &e)
    {
      LOG(LCRITICAL, ("Error:", e.what(), " while creating searchEngine."));
      return nullptr;
    }
  }

  shared_ptr<OsrmRouter> CreateOsrmRouter(shared_ptr<model::FeaturesFetcher> featuresFetcher,
                                          shared_ptr<search::Engine> searchEngine)
  {
    ASSERT(featuresFetcher, ());
    ASSERT(searchEngine, ());

    shared_ptr<OsrmRouter> osrmRouter(new OsrmRouter(
        &featuresFetcher->GetIndex(), [searchEngine](m2::PointD const & pt)
        {
          return searchEngine->GetCountryFile(pt);
        },
        [](string const & countryFileName)
        {
          return make_shared<LocalCountryFile>(LocalCountryFile::MakeForTesting(countryFileName));
        }));
    return osrmRouter;
  }

  class OsrmRouterComponents
  {
  public:
    OsrmRouterComponents(vector<LocalCountryFile> const & localFiles)
        : m_featuresFetcher(CreateFeaturesFetcher(localFiles)),
          m_searchEngine(CreateSearchEngine(m_featuresFetcher)),
          m_osrmRouter(CreateOsrmRouter(m_featuresFetcher, m_searchEngine))
    {
    }
    OsrmRouter * GetOsrmRouter() const { return m_osrmRouter.get(); }
    search::Engine * GetSearchEngine() const { return m_searchEngine.get(); }

  private:
    shared_ptr<model::FeaturesFetcher> m_featuresFetcher;
    shared_ptr<search::Engine> m_searchEngine;
    shared_ptr<OsrmRouter> m_osrmRouter;
  };

  shared_ptr<OsrmRouterComponents> LoadMaps(vector<LocalCountryFile> const & localFiles)
  {
    return shared_ptr<OsrmRouterComponents>(new OsrmRouterComponents(localFiles));
  }

  shared_ptr<OsrmRouterComponents> LoadAllMaps()
  {
    vector<LocalCountryFile> localFiles;
    platform::FindAllLocalMaps(localFiles);
    ASSERT(!localFiles.empty(), ());
    return LoadMaps(localFiles);
  }

  OsrmRouterComponents & GetAllMaps()
  {
    static shared_ptr<OsrmRouterComponents> const inst = LoadAllMaps();
    ASSERT(inst, ());
    return *inst;
  }

  TRouteResult CalculateRoute(OsrmRouterComponents const & routerComponents,
                              m2::PointD const & startPoint, m2::PointD const & startDirection,
                              m2::PointD const & finalPoint)
  {
    OsrmRouter * osrmRouter = routerComponents.GetOsrmRouter();
    ASSERT(osrmRouter, ());
    shared_ptr<Route> route(new Route("mapsme"));
    OsrmRouter::ResultCode result =
        osrmRouter->CalculateRoute(startPoint, startDirection, finalPoint, *route.get());
    ASSERT(route, ());
    return TRouteResult(route, result);
  }

  void TestTurnCount(routing::Route const & route, uint32_t expectedTurnCount)
  {
    TEST_EQUAL(route.GetTurnsGeometry().size(), expectedTurnCount, ());
  }

  void TestRouteLength(Route const & route, double expectedRouteMeters,
                       double relativeError)
  {
    double const delta = expectedRouteMeters * relativeError;
    double const routeMeters = route.GetDistance();
    TEST(my::AlmostEqualAbs(routeMeters, expectedRouteMeters, delta),
        ("Route time test failed. Expected:", expectedRouteMeters, "have:", routeMeters, "delta:", delta));
  }

  void TestRouteTime(Route const & route, double expectedRouteSeconds, double relativeError)
  {
    double const delta = expectedRouteSeconds * relativeError;
    double const routeSeconds = route.GetAllTime();
    TEST(my::AlmostEqualAbs(routeSeconds, expectedRouteSeconds, delta),
        ("Route time test failed. Expected:", expectedRouteSeconds, "have:", routeSeconds, "delta:", delta));
  }

  void CalculateRouteAndTestRouteLength(OsrmRouterComponents const & routerComponents,
                                        m2::PointD const & startPoint,
                                        m2::PointD const & startDirection,
                                        m2::PointD const & finalPoint, double expectedRouteMeters,
                                        double relativeError)
  {
    TRouteResult routeResult =
        CalculateRoute(routerComponents, startPoint, startDirection, finalPoint);
    OsrmRouter::ResultCode const result = routeResult.second;
    TEST_EQUAL(result, OsrmRouter::NoError, ());
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
    turns::TTurnsGeom const & turnsGeom = route.GetTurnsGeometry();
    if (turnNumber >= turnsGeom.size())
      return TestTurn();

    Route::TTurns const & turns = route.GetTurns();
    if (turnNumber >= turns.size())
      return TestTurn();

    turns::TurnGeom const & turnGeom = turnsGeom[turnNumber];
    ASSERT_LESS(turnGeom.m_turnIndex, turnGeom.m_points.size(), ());
    TurnItem const & turn = turns[turnNumber];
    return TestTurn(turnGeom.m_points[turnGeom.m_turnIndex], turn.m_turn, turn.m_exitNum);
  }

  TestTurn GetTurnByPoint(routing::Route const & route, m2::PointD const & approximateTurnPoint,
                          double inaccuracyMeters)
  {
    turns::TTurnsGeom const & turnsGeom = route.GetTurnsGeometry();
    Route::TTurns const & turns = route.GetTurns();
    ASSERT_EQUAL(turnsGeom.size() + 1, turns.size(), ());

    for (int i = 0; i != turnsGeom.size(); ++i)
    {
      turns::TurnGeom const & turnGeom = turnsGeom[i];
      ASSERT_LESS(turnGeom.m_turnIndex, turnGeom.m_points.size(), ());
      m2::PointD const turnPoint = turnGeom.m_points[turnGeom.m_turnIndex];
      if (ms::DistanceOnEarth(turnPoint.y, turnPoint.x, approximateTurnPoint.y, approximateTurnPoint.x) <=
          inaccuracyMeters)
      {
        TurnItem const & turn = turns[i];
        return TestTurn(turnPoint, turn.m_turn, turn.m_exitNum);
      }
    }
    return TestTurn();
  }

  void TestOnlineCrosses(m2::PointD const & startPoint, m2::PointD const & finalPoint,
                         vector<string> const & expected,
                         OsrmRouterComponents & routerComponents)
  {
    routing::OnlineCrossFetcher fetcher(OSRM_ONLINE_SERVER_URL, startPoint, finalPoint);
    vector<m2::PointD> const & points = fetcher.GetMwmPoints();
    TEST_EQUAL(points.size(), expected.size(), ());
    for (m2::PointD const & point : points)
    {
      string const mwmName = routerComponents.GetSearchEngine()->GetCountryFile(point);
      TEST(find(expected.begin(), expected.end(), mwmName) != expected.end(), ("Can't find ", mwmName));
    }
  }
}
