#include "integration_tests/osrm_test_tools.hpp"

#include "testing/testing.hpp"

#include "indexer/index.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "routing/online_cross_fetcher.hpp"
#include "routing/route.hpp"

#include "map/feature_vec_model.hpp"

#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "search/search_engine.hpp"

#include <sys/resource.h>


using namespace routing;

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
  shared_ptr<model::FeaturesFetcher> CreateFeaturesFetcher(vector<string> const & mapNames)
  {
    size_t const maxOpenFileNumber = 1024;
    ChangeMaxNumberOfOpenFiles(maxOpenFileNumber);
    shared_ptr<model::FeaturesFetcher> featuresFetcher(new model::FeaturesFetcher);
    featuresFetcher->InitClassificator();

    for (auto const mapName : mapNames)
    {
      pair<MwmSet::MwmLock, bool> result = featuresFetcher->RegisterMap(mapName);
      if (!result.second)
      {
        ASSERT(false, ());
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

    shared_ptr<OsrmRouter> osrmRouter(new OsrmRouter(&featuresFetcher->GetIndex(),
                                                     [searchEngine](m2::PointD const & pt)
                                                     {
      return searchEngine->GetCountryFile(pt);
    }));
    return osrmRouter;
  }

  class OsrmRouterComponents
  {
  public:
    OsrmRouterComponents(vector<string> const & mapNames)
      : m_featuresFetcher(CreateFeaturesFetcher(mapNames)),
        m_searchEngine(CreateSearchEngine(m_featuresFetcher)),
        m_osrmRouter(CreateOsrmRouter(m_featuresFetcher, m_searchEngine)) {}
    OsrmRouter * GetOsrmRouter() const { return m_osrmRouter.get(); }
    search::Engine * GetSearchEngine() const { return m_searchEngine.get(); }

  private:
    shared_ptr<model::FeaturesFetcher> m_featuresFetcher;
    shared_ptr<search::Engine> m_searchEngine;
    shared_ptr<OsrmRouter> m_osrmRouter;
  };

  void GetMapNames(vector<string> & maps)
  {
    Platform const & pl = GetPlatform();

    pl.GetFilesByExt(pl.ResourcesDir(), DATA_FILE_EXTENSION, maps);
    pl.GetFilesByExt(pl.WritableDir(), DATA_FILE_EXTENSION, maps);

    sort(maps.begin(), maps.end());
    maps.erase(unique(maps.begin(), maps.end()), maps.end());
  }

  shared_ptr<OsrmRouterComponents> LoadMaps(vector<string> const & mapNames)
  {
    return shared_ptr<OsrmRouterComponents>(new OsrmRouterComponents(mapNames));
  }

  shared_ptr<OsrmRouterComponents> LoadAllMaps()
  {
    vector<string> maps;
    GetMapNames(maps);
    ASSERT(!maps.empty(), ());
    return LoadMaps(maps);
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

  void TestRouteLength(Route const & route, double expectedRouteLength,
                       double relativeError)
  {
    double const delta = expectedRouteLength * relativeError;
    double const routeLength = route.GetDistance();
    TEST_LESS_OR_EQUAL(routeLength - delta, expectedRouteLength, ());
    TEST_GREATER_OR_EQUAL(routeLength + delta, expectedRouteLength, ());
  }

  void CalculateRouteAndTestRouteLength(OsrmRouterComponents const & routerComponents,
                                        m2::PointD const & startPoint,
                                        m2::PointD const & startDirection,
                                        m2::PointD const & finalPoint, double expectedRouteLength,
                                        double relativeError)
  {
    TRouteResult routeResult =
        CalculateRoute(routerComponents, startPoint, startDirection, finalPoint);
    OsrmRouter::ResultCode const result = routeResult.second;
    TEST_EQUAL(result, OsrmRouter::NoError, ());
    TestRouteLength(*routeResult.first, expectedRouteLength, relativeError);
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
    double const dist = ms::DistanceOnEarth(expectedPoint.y, expectedPoint.x, m_point.y, m_point.x);
    TEST_LESS(dist, inaccuracyMeters, ());
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

  TestTurn GetNthTurn(routing::Route const & route, uint32_t expectedTurnNumber)
  {
    turns::TurnsGeomT const & turnsGeom = route.GetTurnsGeometry();
    if (expectedTurnNumber >= turnsGeom.size())
      return TestTurn();

    Route::TurnsT const & turns = route.GetTurns();
    if (expectedTurnNumber >= turns.size())
      return TestTurn();

    turns::TurnGeom const & turnGeom = turnsGeom[expectedTurnNumber];
    ASSERT_LESS(turnGeom.m_turnIndex, turnGeom.m_points.size(), ());
    TurnItem const & turn = turns[expectedTurnNumber];
    return TestTurn(turnGeom.m_points[turnGeom.m_turnIndex], turn.m_turn, turn.m_exitNum);
  }

  TestTurn GetTurnByPoint(routing::Route const & route, m2::PointD const & expectedTurnPoint,
                          double inaccuracyMeters)
  {
    turns::TurnsGeomT const & turnsGeom = route.GetTurnsGeometry();
    Route::TurnsT const & turns = route.GetTurns();
    ASSERT_EQUAL(turnsGeom.size() + 1, turns.size(), ());

    for (int i = 0; i != turnsGeom.size(); ++i)
    {
      turns::TurnGeom const & turnGeom = turnsGeom[i];
      ASSERT_LESS(turnGeom.m_turnIndex, turnGeom.m_points.size(), ());
      m2::PointD const turnPoint = turnGeom.m_points[turnGeom.m_turnIndex];
      if (ms::DistanceOnEarth(turnPoint.y, turnPoint.x, expectedTurnPoint.y, expectedTurnPoint.x) <=
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
      TEST(find(expected.begin(), expected.end(), mwmName) != expected.end(), ());
    }
  }
}
