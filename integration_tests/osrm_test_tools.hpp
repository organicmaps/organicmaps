#pragma once

#include "../std/shared_ptr.hpp"
#include "../std/string.hpp"
#include "../std/vector.hpp"

#include "../routing/osrm_router.hpp"

/*
 * These tests are developed to simplify routing integration tests writing.
 * You can use the interface bellow however you want but there are some hints.
 * 1. Most likely you want to use GetAllMaps() to get shared_ptr<OsrmRouterComponents>.
 *    It loads all the maps from directories Platform::WritableDir()
 *    and Platform::ResourcesDir() only once and then reuse it.
 *    Use LoadMaps() only if you want to test something on a special map set.
 * 2. Loading maps and calculating routes is a time consumption process.
 *    Do this only if you really need it.
 * 3. If you want to check that a turn is absent use TestTurnCount.
 * 4. The easiest way to gather all the information for writing an integration test is
 *    - to put a break point in OsrmRouter::CalculateRouteImpl;
 *    - to make a route with MapWithMe desktop application;
 *    - to get all necessary parameters and result of route calculation;
 *    - to place them into the test you're writing.
 * 5. The recommended way for naming tests for route from one place to another one is
 *    <Country><City><Street1><House1><Street2><House2><Test time. TurnTest or RouteTest for the time being>
 */

/// Inaccuracy of turn point in meters
double const turnInaccuracy = 2.;
/// Inaccurace of the route length. It is used for checking if routes have etalon(sample) length.
/// So the a created route has etalon length if
/// etalonLength - etalonRouteLength * routeLengthInaccurace  <= route->GetDistance()
/// && etalonRouteLength + etalonRouteLength * routeLengthInaccurace  >= route->GetDistance()
/// is equal to true.
double const routeLengthInaccurace = .01;

typedef pair<shared_ptr<routing::Route>, routing::OsrmRouter::ResultCode> RouteResultT;

namespace integration
{
  class OsrmRouterComponents;

  shared_ptr<OsrmRouterComponents> GetAllMaps();
  shared_ptr<OsrmRouterComponents> LoadMaps(vector<string> const & mapNames);

  RouteResultT CalculateRoute(shared_ptr<OsrmRouterComponents> routerComponents,
                                   m2::PointD const & startPt, m2::PointD const & startDr, m2::PointD const & finalPt);

  bool TestTurn(shared_ptr<routing::Route> const route, uint32_t etalonTurnNumber, m2::PointD const & etalonTurnPnt,
                 routing::turns::TurnDirection etalonTurnDirection, uint32_t etalonRoundAboutExitNum = 0);

  bool TestTurnCount(shared_ptr<routing::Route> const route, uint32_t etalonTurnCount);

  bool TestRouteLength(shared_ptr<routing::Route> const route, double etalonRouteLength);

  bool CalculateRouteAndTestRouteLength(shared_ptr<OsrmRouterComponents> routerComponents, m2::PointD const & startPt,
                                        m2::PointD const & startDr, m2::PointD const & finalPt, double etalonRouteLength);
}
