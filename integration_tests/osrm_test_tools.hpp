#pragma once

#include "std/shared_ptr.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"
#include "std/set.hpp"

#include "routing/osrm_router.hpp"

/*
 * These tests are developed to simplify routing integration tests writing.
 * You can use the interface bellow however you want but there are some hints.
 * 1. Most likely you want to use GetAllMaps() to get shared_ptr<OsrmRouterComponents>.
 *    It loads all the maps from directories Platform::WritableDir()
 *    and Platform::ResourcesDir() only once and then reuse it.
 *    Use LoadMaps() only if you want to test something on a special map set.
 * 2. Loading maps and calculating routes is a time consumption process.
 *    Do this only if you really need it.
 * 3. If you want to check that a turn is absent you have two options
 *    - use GetTurnByPoint(...).TestNotValid();
 *    - or use TestTurnCount.
 * 4. The easiest way to gather all the information for writing an integration test is
 *    - to put a break point in OsrmRouter::CalculateRouteImpl;
 *    - to make a route with MapWithMe desktop application;
 *    - to get all necessary parameters and result of the route calculation;
 *    - to place them into the test you're writing.
 * 5. The recommended way for naming tests for a route from one place to another one is
 *    <Country><City><Street1><House1><Street2><House2><Test time. TurnTest or RouteTest for the time being>
 * 6. It's a good idea to use short routes for testing turns. The thing is geometry of long routes
 *    could be changes for one dataset (osrm version) to another one.
 *    The shorter route the less chance it'll be changed.
 */

typedef pair<shared_ptr<routing::Route>, routing::OsrmRouter::ResultCode> RouteResultT;

namespace integration
{
  class OsrmRouterComponents;

  void TestOnlineCrosses(m2::PointD const & startPoint, m2::PointD const & finalPoint,
                         vector<string> const & expected,
                         shared_ptr<OsrmRouterComponents> & routerComponents);

  shared_ptr<OsrmRouterComponents> GetAllMaps();
  shared_ptr<OsrmRouterComponents> LoadMaps(vector<string> const & mapNames);
  RouteResultT CalculateRoute(shared_ptr<OsrmRouterComponents> routerComponents,
                              m2::PointD const & startPt, m2::PointD const & startDr, m2::PointD const & finalPt);

  void TestTurnCount(shared_ptr<routing::Route> const route, uint32_t referenceTurnCount);

  /// Testing route length.
  /// It is used for checking if routes have reference(sample) length.
  /// The a created route will pass the test iff
  /// referenceRouteLength - referenceRouteLength * routeLenInaccuracy  <= route->GetDistance()
  /// && referenceRouteLength + referenceRouteLength * routeLenInaccuracy >= route->GetDistance()
  void TestRouteLength(shared_ptr<routing::Route> const route, double referenceRouteLength, double routeLenInaccuracy = 0.01);
  void CalculateRouteAndTestRouteLength(shared_ptr<OsrmRouterComponents> routerComponents, m2::PointD const & startPt,
                                        m2::PointD const & startDr, m2::PointD const & finalPt, double referenceRouteLength,
                                        double routeLenInaccuracy = 0.07);

  class TestTurn
  {
    friend TestTurn GetNthTurn(shared_ptr<routing::Route> const route, uint32_t turnNumber);
    friend TestTurn GetTurnByPoint(shared_ptr<routing::Route> const route, m2::PointD const & turnPnt, double inaccuracy);

    m2::PointD const m_pnt;
    routing::turns::TurnDirection const m_direction;
    uint32_t const m_roundAboutExitNum;
    bool const m_isValid;

    TestTurn() : m_pnt({0., 0.}), m_direction(routing::turns::NoTurn), m_roundAboutExitNum(0), m_isValid(false) {}
    TestTurn(m2::PointD  const & pnt, routing::turns::TurnDirection direction, uint32_t roundAboutExitNum) :
      m_pnt(pnt), m_direction(direction), m_roundAboutExitNum(roundAboutExitNum), m_isValid(true) {}
  public:
    const TestTurn & TestValid() const;
    const TestTurn & TestNotValid() const;
    const TestTurn & TestPoint(m2::PointD  const & referencePnt, double inaccuracy = 3.) const;
    const TestTurn & TestDirection(routing::turns::TurnDirection referenceDirection) const;
    const TestTurn & TestOneOfDirections(set<routing::turns::TurnDirection> referenceDirections) const;
    const TestTurn & TestRoundAboutExitNum(uint32_t referenceRoundAboutExitNum) const;
  };

  /// Extracting appropriate TestTurn if any. If not TestTurn::isValid() returns false.
  /// inaccuracy is set in meters.
  TestTurn GetNthTurn(shared_ptr<routing::Route> const route, uint32_t turnNumber);
  TestTurn GetTurnByPoint(shared_ptr<routing::Route> const route, m2::PointD const & turnPnt, double inaccuracy = 3.);
}
