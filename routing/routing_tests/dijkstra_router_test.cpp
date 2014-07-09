#include "../../testing/testing.hpp"
#include "../dijkstra_router.hpp"
#include "road_graph_builder.hpp"
#include "../../base/logging.hpp"
#include "../../base/macros.hpp"

using namespace routing;
using namespace routing_test;

template <size_t finalPosSize, size_t startPosSize, size_t expectedSize>
void TestDijkstraRouter(RoadPos (&finalPos)[finalPosSize],
                        RoadPos (&startPos)[startPosSize],
                        RoadPos (&expected)[expectedSize])
{
  RoadGraphMockSource graph;
  InitRoadGraphMockSourceWithTest2(graph);
  DijkstraRouter router;
  router.SetRoadGraph(&graph);
  router.SetFinalRoadPos(vector<RoadPos>(&finalPos[0], &finalPos[0] + ARRAY_SIZE(finalPos)));
  vector<RoadPos> result;
  router.CalculateRoute(vector<RoadPos>(&startPos[0], &startPos[0] + ARRAY_SIZE(startPos)), result);
  TEST_EQUAL(vector<RoadPos>(&expected[0], &expected[0] + ARRAY_SIZE(expected)), result, ());
}

UNIT_TEST(Dijkstra_Router_City_Simple)
{
  RoadPos finalPos[] = { RoadPos(7, true, 0) };
  RoadPos startPos[] = { RoadPos(1, true, 0) };
  RoadPos expected[] = { RoadPos(1, true, 0),
                         RoadPos(1, true, 1),
                         RoadPos(8, true, 0),
                         RoadPos(8, true, 1),
                         RoadPos(7, true, 0) };
  TestDijkstraRouter(finalPos, startPos, expected);
}

UNIT_TEST(Dijkstra_Router_City_ReallyFunnyLoop)
{
  // my::g_LogLevel = LDEBUG;

  RoadPos finalPos[] = { RoadPos(1, true, 0) };
  RoadPos startPos[] = { RoadPos(1, true, 1) };
  RoadPos expected[] = { RoadPos(1, true, 1),
                         RoadPos(8, true, 1),
                         RoadPos(8, true, 2),
                         RoadPos(5, false, 0),
                         RoadPos(0, false, 1),
                         RoadPos(0, false, 0),
                         RoadPos(1, true, 0) };
  TestDijkstraRouter(finalPos, startPos, expected);
}
