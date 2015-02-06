#include "osrm_test_tools.hpp"

#include "../std/new.hpp"

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
      m_startPt = startPt;
      m_startDr = startDr;
      m_finalPt = finalPt;
      m_cachedFinalNodes.clear();
      m_isFinalChanged = false;
      m_requestCancel = false;

      return OsrmRouter::CalculateRouteImpl(startPt, startDr, finalPt, route);
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
    try{
      return shared_ptr<OsrmRouterComponents>(new OsrmRouterComponents(mapNames));
    }
    catch(bad_alloc &)
    {
      ASSERT(false, ());
      return nullptr;
    }
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
    try
    {
      ASSERT(routerComponents.get(), ());
      OsrmRouterWrapper * osrmRouter = routerComponents->GetOsrmRouter();
      ASSERT(osrmRouter, ());
      shared_ptr<Route> route(new Route("mapsme"));
      OsrmRouter::ResultCode result = osrmRouter->SyncCalculateRoute(startPt, startDr, finalPt, *route.get());
      return RouteResultT(route, result);
    }
    catch(bad_alloc &)
    {
      ASSERT(false, ());
      return RouteResultT(nullptr, OsrmRouter::InternalError);
    }
  }

  bool TestTurn(shared_ptr<Route> const route, uint32_t etalonTurnNumber, m2::PointD const & etalonTurnPnt,
                turns::TurnDirection etalonTurnDirection, uint32_t etalonRoundAboutExitNum)
  {
    ASSERT(route.get(), ());
    turns::TurnsGeomT const & turnsGeom = route->GetTurnsGeometry();
    if (etalonTurnNumber >= turnsGeom.size())
      return false;

    Route::TurnsT const & turns = route->GetTurns();
    ASSERT_LESS(etalonTurnNumber, turns.size(), ());
    Route::TurnItem const & turn = turns[etalonTurnNumber];
    if (turn.m_turn != etalonTurnDirection)
      return false;
    if (turn.m_exitNum != etalonRoundAboutExitNum)
      return false;

    turns::TurnGeom const & turnGeom = turnsGeom[etalonTurnNumber];
    ASSERT_LESS(turnGeom.m_turnIndex, turnGeom.m_points.size(), ());
    m2::PointD turnGeomPnt = turnGeom.m_points[turnGeom.m_turnIndex];

    double const dist = ms::DistanceOnEarth(etalonTurnPnt.y, etalonTurnPnt.x, turnGeomPnt.y, turnGeomPnt.x);
    if (dist > turnInaccuracy)
      return false;
    return true;
  }

  bool TestTurnCount(shared_ptr<routing::Route> const route, uint32_t etalonTurnCount)
  {
    ASSERT(route.get(), ());
    return route->GetTurnsGeometry().size() == etalonTurnCount;
  }

  bool TestRouteLength(shared_ptr<Route> const route, double etalonRouteLength)
  {
    ASSERT(route.get(), ());
    double const delta = etalonRouteLength * routeLengthInaccurace;
    double const routeLength = route->GetDistance();
    if (routeLength - delta <= etalonRouteLength && routeLength + delta >= etalonRouteLength)
      return true;
    else
      return false;
  }

  bool CalculateRouteAndTestRouteLength(shared_ptr<OsrmRouterComponents> routerComponents, m2::PointD const & startPt,
                                        m2::PointD const & startDr, m2::PointD const & finalPt, double etalonRouteLength)
  {
    RouteResultT routeResult = CalculateRoute(routerComponents, startPt, startDr, finalPt);
    shared_ptr<Route> const route = routeResult.first;
    OsrmRouter::ResultCode const result = routeResult.second;
    if (result != OsrmRouter::NoError)
      return false;
    return TestRouteLength(route, etalonRouteLength);
  }
}
