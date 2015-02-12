#include "osrm_test_tools.hpp"

#include "../../testing/testing.hpp"

#include "../indexer/index.hpp"

#include "../geometry/distance_on_sphere.hpp"

#include "../routing/route.hpp"

#include "../map/feature_vec_model.hpp"

#include "../platform/platform.hpp"
#include "../platform/preferred_languages.hpp"

#include "../search/search_engine.hpp"


using namespace routing;

namespace integration
{
  class OsrmRouterWrapper : public OsrmRouter
  {
  public:
    OsrmRouterWrapper(Index const * index, CountryFileFnT const & fn) :
      OsrmRouter(index, fn) {}
    ResultCode SyncCalculateRoute(m2::PointD const & startPt, m2::PointD const & startDr, m2::PointD const & finalPt, Route & route)
    {
      SetFinalPoint(finalPt);
      return CalculateRouteImpl(startPt, startDr, finalPt, route);
    }
  };

  shared_ptr<search::Engine> CreateSearchEngine(search::Engine::IndexType const * pIndex)
  {
    ASSERT(pIndex, ());
    Platform const & pl = GetPlatform();
    try
    {
      shared_ptr<search::Engine> searchEngine(new search::Engine(
                         pIndex,
                         pl.GetReader(SEARCH_CATEGORIES_FILE_NAME),
                         pl.GetReader(PACKED_POLYGONS_FILE),
                         pl.GetReader(COUNTRIES_FILE),
                         languages::GetCurrentOrig()));
      return searchEngine;
    }
    catch (RootException const &)
    {
      ASSERT(false, ());
      return nullptr;
    }
  }

  shared_ptr<model::FeaturesFetcher> CreateFeaturesFetcher(vector<string> const & mapNames)
  {
    shared_ptr<model::FeaturesFetcher> featuresFetcher(new model::FeaturesFetcher);
    featuresFetcher->InitClassificator();

    for (auto const mapName : mapNames)
    {
      if (featuresFetcher->AddMap(mapName) == -1)
      {
        ASSERT(false, ());
        return nullptr;
      }
    }
    return featuresFetcher;
  }

  shared_ptr<search::Engine> CreateSearchEngine(shared_ptr<model::FeaturesFetcher> featuresFetcher)
  {
    shared_ptr<search::Engine> searchEngine = CreateSearchEngine(&featuresFetcher->GetIndex());
    if (!searchEngine.get())
      ASSERT(false, ());
    return searchEngine;
  }

  shared_ptr<OsrmRouterWrapper> CreateOsrmRouter(shared_ptr<model::FeaturesFetcher> featuresFetcher,
                                             shared_ptr<search::Engine> searchEngine)
  {
    shared_ptr<OsrmRouterWrapper> osrmRouter(new OsrmRouterWrapper(&featuresFetcher->GetIndex(),
                                                     [searchEngine]  (m2::PointD const & pt)
    {
      return searchEngine->GetCountryFile(pt);
    }));
    return osrmRouter;
  }

  class OsrmRouterComponents
  {
  public:
    OsrmRouterComponents(vector<string> const & mapNames) :
      m_featuresFetcher(CreateFeaturesFetcher(mapNames)),
      m_searchEngine(CreateSearchEngine(m_featuresFetcher)),
      m_osrmRouter(CreateOsrmRouter(m_featuresFetcher, m_searchEngine)) {}
    OsrmRouterWrapper * GetOsrmRouter() const { return m_osrmRouter.get(); }
  private:
    shared_ptr<model::FeaturesFetcher> m_featuresFetcher;
    shared_ptr<search::Engine> m_searchEngine;
    shared_ptr<OsrmRouterWrapper> m_osrmRouter;
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

  shared_ptr<OsrmRouterComponents> GetAllMaps()
  {
    static shared_ptr<OsrmRouterComponents> inst = LoadAllMaps();
    ASSERT(inst.get(), ());
    return inst;
  }

  RouteResultT CalculateRoute(shared_ptr<OsrmRouterComponents> routerComponents,
                              m2::PointD const & startPt, m2::PointD const & startDr, m2::PointD const & finalPt)
  {
    ASSERT(routerComponents.get(), ());
    OsrmRouterWrapper * osrmRouter = routerComponents->GetOsrmRouter();
    ASSERT(osrmRouter, ());
    shared_ptr<Route> route(new Route("mapsme"));
    OsrmRouter::ResultCode result = osrmRouter->SyncCalculateRoute(startPt, startDr, finalPt, *route.get());
    return RouteResultT(route, result);
  }

  void TestTurnCount(shared_ptr<routing::Route> const route, uint32_t referenceTurnCount)
  {
    ASSERT(route.get(), ());
    TEST_EQUAL(route->GetTurnsGeometry().size(), referenceTurnCount, ());
  }

  void TestRouteLength(shared_ptr<Route> const route, double referenceRouteLength, double routeLenInaccuracy)
  {
    ASSERT(route.get(), ());
    double const delta = referenceRouteLength * routeLenInaccuracy;
    double const routeLength = route->GetDistance();
    TEST_LESS_OR_EQUAL(routeLength - delta, referenceRouteLength, ());
    TEST_GREATER_OR_EQUAL(routeLength + delta, referenceRouteLength, ());
  }

  void CalculateRouteAndTestRouteLength(shared_ptr<OsrmRouterComponents> routerComponents, m2::PointD const & startPt,
                                        m2::PointD const & startDr, m2::PointD const & finalPt, double referenceRouteLength,
                                        double routeLenInaccuracy)
  {
    RouteResultT routeResult = CalculateRoute(routerComponents, startPt, startDr, finalPt);
    shared_ptr<Route> const route = routeResult.first;
    OsrmRouter::ResultCode const result = routeResult.second;
    TEST_EQUAL(result, OsrmRouter::NoError, ());
    TestRouteLength(route, referenceRouteLength, routeLenInaccuracy);
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

  const TestTurn & TestTurn::TestPoint(m2::PointD const & referencePnt, double inaccuracy) const
  {
    double const dist = ms::DistanceOnEarth(referencePnt.y, referencePnt.x, m_pnt.y, m_pnt.x);
    TEST_LESS(dist, inaccuracy, ());
    return *this;
  }

  const TestTurn & TestTurn::TestDirection(routing::turns::TurnDirection referenceDirection) const
  {
    TEST_EQUAL(m_direction, referenceDirection, ());
    return *this;
  }

  const TestTurn & TestTurn::TestOneOfDirections(set<routing::turns::TurnDirection> referenceDirections) const
  {
    TEST(referenceDirections.find(m_direction) != referenceDirections.end(), ());
    return *this;
  }

  const TestTurn & TestTurn::TestRoundAboutExitNum(uint32_t referenceRoundAboutExitNum) const
  {
    TEST_EQUAL(m_roundAboutExitNum, referenceRoundAboutExitNum, ());
    return *this;
  }

  TestTurn GetNthTurn(shared_ptr<routing::Route> const route, uint32_t referenceTurnNumber)
  {
    ASSERT(route.get(), ());

    turns::TurnsGeomT const & turnsGeom = route->GetTurnsGeometry();
    if (referenceTurnNumber >= turnsGeom.size())
      return TestTurn();

    Route::TurnsT const & turns = route->GetTurns();
    if (referenceTurnNumber >= turns.size())
      return TestTurn();

    turns::TurnGeom const & turnGeom = turnsGeom[referenceTurnNumber];
    ASSERT_LESS(turnGeom.m_turnIndex, turnGeom.m_points.size(), ());
    Route::TurnItem const & turn = turns[referenceTurnNumber];
    return TestTurn(turnGeom.m_points[turnGeom.m_turnIndex], turn.m_turn, turn.m_exitNum);
  }

  TestTurn GetTurnByPoint(shared_ptr<routing::Route> const route, m2::PointD const & referenceTurnPnt, double inaccuracy)
  {
    ASSERT(route.get(), ());
    turns::TurnsGeomT const & turnsGeom = route->GetTurnsGeometry();
    Route::TurnsT const & turns = route->GetTurns();
    ASSERT_EQUAL(turnsGeom.size() + 1, turns.size(), ());

    for (int i = 0; i != turnsGeom.size(); ++i)
    {
      turns::TurnGeom const & turnGeom = turnsGeom[i];
      ASSERT_LESS(turnGeom.m_turnIndex, turnGeom.m_points.size(), ());
      m2::PointD const turnPnt = turnGeom.m_points[turnGeom.m_turnIndex];
      if (ms::DistanceOnEarth(turnPnt.y, turnPnt.x, referenceTurnPnt.y, referenceTurnPnt.x) <= inaccuracy)
      {
        Route::TurnItem const & turn = turns[i];
        return TestTurn(turnPnt, turn.m_turn, turn.m_exitNum);
      }
    }
    return TestTurn();
  }
}
