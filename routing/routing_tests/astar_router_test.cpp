#include "testing/testing.hpp"

#include "routing/routing_tests/road_graph_builder.hpp"

#include "routing/routing_algorithm.hpp"
#include "routing/features_road_graph.hpp"
#include "routing/route.hpp"
#include "routing/router_delegate.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/feature_altitude.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"

using namespace routing;
using namespace routing_test;

namespace
{

using TRoutingAlgorithm = AStarBidirectionalRoutingAlgorithm;

void TestAStarRouterMock(Junction const & startPos, Junction const & finalPos,
                         vector<Junction> const & expected)
{
  classificator::Load();

  RoadGraphMockSource graph;
  InitRoadGraphMockSourceWithTest2(graph);

  RouterDelegate delegate;
  RoutingResult<Junction, double /* Weight */> result;
  TRoutingAlgorithm algorithm;
  TEST_EQUAL(TRoutingAlgorithm::Result::OK,
             algorithm.CalculateRoute(graph, startPos, finalPos, delegate, result), ());

  TEST_EQUAL(expected, result.m_path, ());
}

void AddRoad(RoadGraphMockSource & graph, double speedKMPH, initializer_list<m2::PointD> const & points)
{
  graph.AddRoad(routing::MakeRoadInfoForTesting(true /* bidir */, speedKMPH, points));
}

void AddRoad(RoadGraphMockSource & graph, initializer_list<m2::PointD> const & points)
{
  double const speedKMPH = graph.GetMaxSpeedKMPH();
  graph.AddRoad(routing::MakeRoadInfoForTesting(true /* bidir */, speedKMPH, points));
}

}  // namespace

UNIT_TEST(AStarRouter_Graph2_Simple1)
{
  Junction const startPos = MakeJunctionForTesting(m2::PointD(0, 0));
  Junction const finalPos = MakeJunctionForTesting(m2::PointD(80, 55));

  vector<Junction> const expected = {
      MakeJunctionForTesting(m2::PointD(0, 0)),   MakeJunctionForTesting(m2::PointD(5, 10)),
      MakeJunctionForTesting(m2::PointD(5, 40)),  MakeJunctionForTesting(m2::PointD(18, 55)),
      MakeJunctionForTesting(m2::PointD(39, 55)), MakeJunctionForTesting(m2::PointD(80, 55))};

  TestAStarRouterMock(startPos, finalPos, expected);
}

UNIT_TEST(AStarRouter_Graph2_Simple2)
{
  Junction const startPos = MakeJunctionForTesting(m2::PointD(80, 55));
  Junction const finalPos = MakeJunctionForTesting(m2::PointD(80, 0));

  vector<Junction> const expected = {
      MakeJunctionForTesting(m2::PointD(80, 55)), MakeJunctionForTesting(m2::PointD(39, 55)),
      MakeJunctionForTesting(m2::PointD(37, 30)), MakeJunctionForTesting(m2::PointD(70, 30)),
      MakeJunctionForTesting(m2::PointD(70, 10)), MakeJunctionForTesting(m2::PointD(70, 0)),
      MakeJunctionForTesting(m2::PointD(80, 0))};

  TestAStarRouterMock(startPos, finalPos, expected);
}

UNIT_TEST(AStarRouter_SimpleGraph_RouteIsFound)
{
  classificator::Load();

  RoadGraphMockSource graph;
  AddRoad(graph, {m2::PointD(0, 0), m2::PointD(40, 0)}); // feature 0
  AddRoad(graph, {m2::PointD(40, 0), m2::PointD(40, 30)}); // feature 1
  AddRoad(graph, {m2::PointD(40, 30), m2::PointD(40, 100)}); // feature 2
  AddRoad(graph, {m2::PointD(40, 100), m2::PointD(0, 60)}); // feature 3
  AddRoad(graph, {m2::PointD(0, 60), m2::PointD(0, 30)}); // feature 4
  AddRoad(graph, {m2::PointD(0, 30), m2::PointD(0, 0)}); // feature 5

  Junction const startPos = MakeJunctionForTesting(m2::PointD(0, 0));
  Junction const finalPos = MakeJunctionForTesting(m2::PointD(40, 100));

  vector<Junction> const expected = {
      MakeJunctionForTesting(m2::PointD(0, 0)), MakeJunctionForTesting(m2::PointD(0, 30)),
      MakeJunctionForTesting(m2::PointD(0, 60)), MakeJunctionForTesting(m2::PointD(40, 100))};

  RouterDelegate delegate;
  RoutingResult<Junction, double /* Weight */> result;
  TRoutingAlgorithm algorithm;
  TEST_EQUAL(TRoutingAlgorithm::Result::OK,
             algorithm.CalculateRoute(graph, startPos, finalPos, delegate, result), ());

  TEST_EQUAL(expected, result.m_path, ());
}

UNIT_TEST(AStarRouter_SimpleGraph_RoutesInConnectedComponents)
{
  classificator::Load();

  RoadGraphMockSource graph;

  double const speedKMPH = graph.GetMaxSpeedKMPH();

  // Roads in the first connected component.
  vector<IRoadGraph::RoadInfo> const roadInfo_1 = {
      IRoadGraph::RoadInfo(true /* bidir */, speedKMPH,
                           {MakeJunctionForTesting(m2::PointD(10, 10)),
                            MakeJunctionForTesting(m2::PointD(90, 10))}),  // feature 0
      IRoadGraph::RoadInfo(true /* bidir */, speedKMPH,
                           {MakeJunctionForTesting(m2::PointD(90, 10)),
                            MakeJunctionForTesting(m2::PointD(90, 90))}),  // feature 1
      IRoadGraph::RoadInfo(true /* bidir */, speedKMPH,
                           {MakeJunctionForTesting(m2::PointD(90, 90)),
                            MakeJunctionForTesting(m2::PointD(10, 90))}),  // feature 2
      IRoadGraph::RoadInfo(true /* bidir */, speedKMPH,
                           {MakeJunctionForTesting(m2::PointD(10, 90)),
                            MakeJunctionForTesting(m2::PointD(10, 10))}),  // feature 3
  };
  vector<uint32_t> const featureId_1 = { 0, 1, 2, 3 }; // featureIDs in the first connected component

  // Roads in the second connected component.
  vector<IRoadGraph::RoadInfo> const roadInfo_2 = {
      IRoadGraph::RoadInfo(true /* bidir */, speedKMPH,
                           {MakeJunctionForTesting(m2::PointD(30, 30)),
                            MakeJunctionForTesting(m2::PointD(70, 30))}),  // feature 4
      IRoadGraph::RoadInfo(true /* bidir */, speedKMPH,
                           {MakeJunctionForTesting(m2::PointD(70, 30)),
                            MakeJunctionForTesting(m2::PointD(70, 70))}),  // feature 5
      IRoadGraph::RoadInfo(true /* bidir */, speedKMPH,
                           {MakeJunctionForTesting(m2::PointD(70, 70)),
                            MakeJunctionForTesting(m2::PointD(30, 70))}),  // feature 6
      IRoadGraph::RoadInfo(true /* bidir */, speedKMPH,
                           {MakeJunctionForTesting(m2::PointD(30, 70)),
                            MakeJunctionForTesting(m2::PointD(30, 30))}),  // feature 7
  };
  vector<uint32_t> const featureId_2 = { 4, 5, 6, 7 }; // featureIDs in the second connected component

  for (auto const & ri : roadInfo_1)
    graph.AddRoad(IRoadGraph::RoadInfo(ri));

  for (auto const & ri : roadInfo_2)
    graph.AddRoad(IRoadGraph::RoadInfo(ri));

  TRoutingAlgorithm algorithm;

  // In this test we check that there is no any route between pairs from different connected components,
  // but there are routes between points in one connected component.

  // Check if there is no any route between points in different connected components.
  for (size_t i = 0; i < roadInfo_1.size(); ++i)
  {
    Junction const startPos = roadInfo_1[i].m_junctions[0];
    for (size_t j = 0; j < roadInfo_2.size(); ++j)
    {
      RouterDelegate delegate;
      Junction const finalPos = roadInfo_2[j].m_junctions[0];
      RoutingResult<Junction, double /* Weight */> result;
      TEST_EQUAL(TRoutingAlgorithm::Result::NoPath,
                 algorithm.CalculateRoute(graph, startPos, finalPos, delegate, result), ());
      TEST_EQUAL(TRoutingAlgorithm::Result::NoPath,
                 algorithm.CalculateRoute(graph, finalPos, startPos, delegate, result), ());
    }
  }

  // Check if there is route between points in the first connected component.
  for (size_t i = 0; i < roadInfo_1.size(); ++i)
  {
    Junction const startPos = roadInfo_1[i].m_junctions[0];
    for (size_t j = i + 1; j < roadInfo_1.size(); ++j)
    {
      RouterDelegate delegate;
      Junction const finalPos = roadInfo_1[j].m_junctions[0];
      RoutingResult<Junction, double /* Weight */> result;
      TEST_EQUAL(TRoutingAlgorithm::Result::OK,
                 algorithm.CalculateRoute(graph, startPos, finalPos, delegate, result), ());
      TEST_EQUAL(TRoutingAlgorithm::Result::OK,
                 algorithm.CalculateRoute(graph, finalPos, startPos, delegate, result), ());
    }
  }

  // Check if there is route between points in the second connected component.
  for (size_t i = 0; i < roadInfo_2.size(); ++i)
  {
    Junction const startPos = roadInfo_2[i].m_junctions[0];
    for (size_t j = i + 1; j < roadInfo_2.size(); ++j)
    {
      RouterDelegate delegate;
      Junction const finalPos = roadInfo_2[j].m_junctions[0];
      RoutingResult<Junction, double /* Weight */> result;
      TEST_EQUAL(TRoutingAlgorithm::Result::OK,
                 algorithm.CalculateRoute(graph, startPos, finalPos, delegate, result), ());
      TEST_EQUAL(TRoutingAlgorithm::Result::OK,
                 algorithm.CalculateRoute(graph, finalPos, startPos, delegate, result), ());
    }
  }
}

UNIT_TEST(AStarRouter_SimpleGraph_PickTheFasterRoad1)
{
  classificator::Load();

  RoadGraphMockSource graph;

  AddRoad(graph, 5.0, {m2::PointD(2,1), m2::PointD(2,2), m2::PointD(2,3)});
  AddRoad(graph, 5.0, {m2::PointD(10,1), m2::PointD(10,2), m2::PointD(10,3)});

  AddRoad(graph, 5.0, {m2::PointD(2,3), m2::PointD(4,3), m2::PointD(6,3), m2::PointD(8,3), m2::PointD(10,3)});
  AddRoad(graph, 3.0, {m2::PointD(2,2), m2::PointD(6,2), m2::PointD(10,2)});
  AddRoad(graph, 4.0, {m2::PointD(2,1), m2::PointD(10,1)});


  // path1 = 1/5 + 8/5 + 1/5 = 2
  // path2 = 8/3 = 2.666(6)
  // path3 = 1/5 + 8/4 + 1/5 = 2.4

  RouterDelegate delegate;
  RoutingResult<Junction, double /* Weight */> result;
  TRoutingAlgorithm algorithm;
  TEST_EQUAL(TRoutingAlgorithm::Result::OK,
             algorithm.CalculateRoute(graph, MakeJunctionForTesting(m2::PointD(2, 2)),
                                      MakeJunctionForTesting(m2::PointD(10, 2)), delegate, result),
             ());
  TEST_EQUAL(
      result.m_path,
      vector<Junction>(
          {MakeJunctionForTesting(m2::PointD(2, 2)), MakeJunctionForTesting(m2::PointD(2, 3)),
           MakeJunctionForTesting(m2::PointD(4, 3)), MakeJunctionForTesting(m2::PointD(6, 3)),
           MakeJunctionForTesting(m2::PointD(8, 3)), MakeJunctionForTesting(m2::PointD(10, 3)),
           MakeJunctionForTesting(m2::PointD(10, 2))}),
      ());
  TEST(my::AlmostEqualAbs(result.m_distance, 800451., 1.), ("Distance error:", result.m_distance));
}

UNIT_TEST(AStarRouter_SimpleGraph_PickTheFasterRoad2)
{
  classificator::Load();

  RoadGraphMockSource graph;

  AddRoad(graph, 5.0, {m2::PointD(2,1), m2::PointD(2,2), m2::PointD(2,3)});
  AddRoad(graph, 5.0, {m2::PointD(10,1), m2::PointD(10,2), m2::PointD(10,3)});

  AddRoad(graph, 5.0, {m2::PointD(2,3), m2::PointD(4,3), m2::PointD(6,3), m2::PointD(8,3), m2::PointD(10,3)});
  AddRoad(graph, 4.1, {m2::PointD(2,2), m2::PointD(6,2), m2::PointD(10,2)});
  AddRoad(graph, 4.4, {m2::PointD(2,1), m2::PointD(10,1)});

  // path1 = 1/5 + 8/5 + 1/5 = 2
  // path2 = 8/4.1 = 1.95
  // path3 = 1/5 + 8/4.4 + 1/5 = 2.2

  RouterDelegate delegate;
  RoutingResult<Junction, double /* Weight */> result;
  TRoutingAlgorithm algorithm;
  TEST_EQUAL(TRoutingAlgorithm::Result::OK,
             algorithm.CalculateRoute(graph, MakeJunctionForTesting(m2::PointD(2, 2)),
                                      MakeJunctionForTesting(m2::PointD(10, 2)), delegate, result),
             ());
  TEST_EQUAL(result.m_path,
             vector<Junction>({MakeJunctionForTesting(m2::PointD(2, 2)),
                               MakeJunctionForTesting(m2::PointD(6, 2)),
                               MakeJunctionForTesting(m2::PointD(10, 2))}),
             ());
  TEST(my::AlmostEqualAbs(result.m_distance, 781458., 1.), ("Distance error:", result.m_distance));
}

UNIT_TEST(AStarRouter_SimpleGraph_PickTheFasterRoad3)
{
  classificator::Load();

  RoadGraphMockSource graph;

  AddRoad(graph, 5.0, {m2::PointD(2,1), m2::PointD(2,2), m2::PointD(2,3)});
  AddRoad(graph, 5.0, {m2::PointD(10,1), m2::PointD(10,2), m2::PointD(10,3)});

  AddRoad(graph, 4.8, {m2::PointD(2,3), m2::PointD(4,3), m2::PointD(6,3), m2::PointD(8,3), m2::PointD(10,3)});
  AddRoad(graph, 3.9, {m2::PointD(2,2), m2::PointD(6,2), m2::PointD(10,2)});
  AddRoad(graph, 4.9, {m2::PointD(2,1), m2::PointD(10,1)});

  // path1 = 1/5 + 8/4.8 + 1/5 = 2.04
  // path2 = 8/3.9 = 2.05
  // path3 = 1/5 + 8/4.9 + 1/5 = 2.03

  RouterDelegate delegate;
  RoutingResult<Junction, double /* Weight */> result;
  TRoutingAlgorithm algorithm;
  TEST_EQUAL(TRoutingAlgorithm::Result::OK,
             algorithm.CalculateRoute(graph, MakeJunctionForTesting(m2::PointD(2, 2)),
                                      MakeJunctionForTesting(m2::PointD(10, 2)), delegate, result),
             ());
  TEST_EQUAL(
      result.m_path,
      vector<Junction>(
          {MakeJunctionForTesting(m2::PointD(2, 2)), MakeJunctionForTesting(m2::PointD(2, 1)),
           MakeJunctionForTesting(m2::PointD(10, 1)), MakeJunctionForTesting(m2::PointD(10, 2))}),
      ());
  TEST(my::AlmostEqualAbs(result.m_distance, 814412., 1.), ("Distance error:", result.m_distance));
}
