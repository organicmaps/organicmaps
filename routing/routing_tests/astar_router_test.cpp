#include "testing/testing.hpp"

#include "routing/routing_tests/road_graph_builder.hpp"
#include "routing/routing_tests/features_road_graph_test.hpp"

#include "routing/astar_router.hpp"
#include "routing/features_road_graph.hpp"
#include "routing/route.hpp"

#include "indexer/classificator_loader.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"

using namespace routing;
using namespace routing_test;

void TestAStarRouterMock(RoadPos const & startPos, RoadPos const & finalPos,
                         m2::PolylineD const & expected)
{
  AStarRouter router;
  {
    unique_ptr<RoadGraphMockSource> graph(new RoadGraphMockSource());
    InitRoadGraphMockSourceWithTest2(*graph);
    router.SetRoadGraph(move(graph));
  }
  vector<RoadPos> result;
  TEST_EQUAL(IRouter::NoError, router.CalculateRoute(startPos, finalPos, result), ());

  Route route(router.GetName());
  router.GetGraph()->ReconstructPath(result, route);
  TEST_EQUAL(expected, route.GetPoly(), ());
}

UNIT_TEST(AStarRouter_Graph2_Simple1)
{
  classificator::Load();

  RoadPos startPos(1, true /* forward */, 0, m2::PointD(0, 0));
  RoadPos finalPos(7, true /* forward */, 0, m2::PointD(80, 55));

  m2::PolylineD expected = {m2::PointD(0, 0),   m2::PointD(5, 10),  m2::PointD(5, 40),
                            m2::PointD(18, 55), m2::PointD(39, 55), m2::PointD(80, 55)};
  TestAStarRouterMock(startPos, finalPos, expected);
}

UNIT_TEST(AStarRouter_Graph2_Simple2)
{
  RoadPos startPos(7, false /* forward */, 0, m2::PointD(80, 55));
  RoadPos finalPos(0, true /* forward */, 4, m2::PointD(80, 0));
  m2::PolylineD expected = {m2::PointD(80, 55), m2::PointD(39, 55), m2::PointD(37, 30),
                            m2::PointD(70, 30), m2::PointD(70, 10), m2::PointD(70, 0),
                            m2::PointD(80, 0)};
  TestAStarRouterMock(startPos, finalPos, expected);
}
