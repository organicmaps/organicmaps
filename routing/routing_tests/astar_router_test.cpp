#include "testing/testing.hpp"

#include "routing/routing_tests/road_graph_builder.hpp"

#include "routing/astar_router.hpp"
#include "routing/features_road_graph.hpp"
#include "routing/route.hpp"

#include "indexer/classificator_loader.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"

using namespace routing;
using namespace routing_test;

namespace
{

void TestAStarRouterMock(Junction const & startPos, Junction const & finalPos,
                         m2::PolylineD const & expected)
{
  classificator::Load();

  AStarRouter router([](m2::PointD const & /*point*/)
                     {
                       return "Dummy_map.mwm";
                     });
  {
    unique_ptr<RoadGraphMockSource> graph(new RoadGraphMockSource());
    InitRoadGraphMockSourceWithTest2(*graph);
    router.SetRoadGraph(move(graph));
  }

  vector<Junction> result;
  TEST_EQUAL(IRouter::NoError, router.CalculateRoute(startPos, finalPos, result), ());

  Route route(router.GetName());
  router.GetGraph()->ReconstructPath(result, route);
  TEST_EQUAL(expected, route.GetPoly(), ());
}

void AddRoad(RoadGraphMockSource & graph, double speedKMPH, initializer_list<m2::PointD> const & points)
{
  graph.AddRoad(IRoadGraph::RoadInfo(true /* bidir */, speedKMPH, points));
}

void AddRoad(RoadGraphMockSource & graph, initializer_list<m2::PointD> const & points)
{
  double const speedKMPH = graph.GetMaxSpeedKMPH();
  graph.AddRoad(IRoadGraph::RoadInfo(true /* bidir */, speedKMPH, points));
}

}  // namespace

UNIT_TEST(AStarRouter_Graph2_Simple1)
{
  Junction const startPos = m2::PointD(0, 0);
  Junction const finalPos = m2::PointD(80, 55);

  m2::PolylineD const expected = {m2::PointD(0, 0),   m2::PointD(5, 10),  m2::PointD(5, 40),
                                  m2::PointD(18, 55), m2::PointD(39, 55), m2::PointD(80, 55)};

  TestAStarRouterMock(startPos, finalPos, expected);
}

UNIT_TEST(AStarRouter_Graph2_Simple2)
{
  Junction const startPos = m2::PointD(80, 55);
  Junction const finalPos = m2::PointD(80, 0);

  m2::PolylineD const expected = {m2::PointD(80, 55), m2::PointD(39, 55), m2::PointD(37, 30),
                                  m2::PointD(70, 30), m2::PointD(70, 10), m2::PointD(70, 0),
                                  m2::PointD(80, 0)};

  TestAStarRouterMock(startPos, finalPos, expected);
}

UNIT_TEST(AStarRouter_SimpleGraph_RouteIsFound)
{
  classificator::Load();

  AStarRouter router([](m2::PointD const & /*point*/)
                     {
                       return "Dummy_map.mwm";
                     });
  {
    unique_ptr<RoadGraphMockSource> graph(new RoadGraphMockSource());

    AddRoad(*graph, {m2::PointD(0, 0), m2::PointD(40, 0)}); // feature 0
    AddRoad(*graph, {m2::PointD(40, 0), m2::PointD(40, 30)}); // feature 1
    AddRoad(*graph, {m2::PointD(40, 30), m2::PointD(40, 100)}); // feature 2
    AddRoad(*graph, {m2::PointD(40, 100), m2::PointD(0, 60)}); // feature 3
    AddRoad(*graph, {m2::PointD(0, 60), m2::PointD(0, 30)}); // feature 4
    AddRoad(*graph, {m2::PointD(0, 30), m2::PointD(0, 0)}); // feature 5

    router.SetRoadGraph(move(graph));
  }

  Junction const start = m2::PointD(0, 0);
  Junction const finish = m2::PointD(40, 100);

  m2::PolylineD const expected = {m2::PointD(0, 0), m2::PointD(0, 30), m2::PointD(0, 60), m2::PointD(40, 100)};

  vector<Junction> result;
  TEST_EQUAL(IRouter::NoError, router.CalculateRoute(start, finish, result), ());

  Route route(router.GetName());
  router.GetGraph()->ReconstructPath(result, route);
  TEST_EQUAL(expected, route.GetPoly(), ());
}

UNIT_TEST(AStarRouter_SimpleGraph_RoutesInConnectedComponents)
{
  classificator::Load();

  unique_ptr<RoadGraphMockSource> graph(new RoadGraphMockSource());

  double const speedKMPH = graph->GetMaxSpeedKMPH();

  // Roads in the first connected component.
  vector<IRoadGraph::RoadInfo> const roadInfo_1 =
  {
    IRoadGraph::RoadInfo(true /* bidir */, speedKMPH, {m2::PointD(10, 10), m2::PointD(90, 10)}), // feature 0
    IRoadGraph::RoadInfo(true /* bidir */, speedKMPH, {m2::PointD(90, 10), m2::PointD(90, 90)}), // feature 1
    IRoadGraph::RoadInfo(true /* bidir */, speedKMPH, {m2::PointD(90, 90), m2::PointD(10, 90)}), // feature 2
    IRoadGraph::RoadInfo(true /* bidir */, speedKMPH, {m2::PointD(10, 90), m2::PointD(10, 10)}), // feature 3
  };
  vector<uint32_t> const featureId_1 = { 0, 1, 2, 3 }; // featureIDs in the first connected component

  // Roads in the second connected component.
  vector<IRoadGraph::RoadInfo> const roadInfo_2 =
  {
    IRoadGraph::RoadInfo(true /* bidir */, speedKMPH, {m2::PointD(30, 30), m2::PointD(70, 30)}), // feature 4
    IRoadGraph::RoadInfo(true /* bidir */, speedKMPH, {m2::PointD(70, 30), m2::PointD(70, 70)}), // feature 5
    IRoadGraph::RoadInfo(true /* bidir */, speedKMPH, {m2::PointD(70, 70), m2::PointD(30, 70)}), // feature 6
    IRoadGraph::RoadInfo(true /* bidir */, speedKMPH, {m2::PointD(30, 70), m2::PointD(30, 30)}), // feature 7
  };
  vector<uint32_t> const featureId_2 = { 4, 5, 6, 7 }; // featureIDs in the second connected component

  for (auto const & ri : roadInfo_1)
    graph->AddRoad(IRoadGraph::RoadInfo(ri));

  for (auto const & ri : roadInfo_2)
    graph->AddRoad(IRoadGraph::RoadInfo(ri));

  AStarRouter router([](m2::PointD const & /*point*/)
                     {
                       return "Dummy_map.mwm";
                     });
  router.SetRoadGraph(move(graph));

  // In this test we check that there is no any route between pairs from different connected components,
  // but there are routes between points in one connected component.

  // Check if there is no any route between points in different connected components.
  for (size_t i = 0; i < roadInfo_1.size(); ++i)
  {
    Junction const start(roadInfo_1[i].m_points[0]);
    for (size_t j = 0; j < roadInfo_2.size(); ++j)
    {
      Junction const finish(roadInfo_2[j].m_points[0]);
      vector<Junction> route;
      TEST_EQUAL(IRouter::RouteNotFound, router.CalculateRoute(start, finish, route), ());
      TEST_EQUAL(IRouter::RouteNotFound, router.CalculateRoute(finish, start, route), ());
    }
  }

  // Check if there is route between points in the first connected component.
  for (size_t i = 0; i < roadInfo_1.size(); ++i)
  {
    Junction const start(roadInfo_1[i].m_points[0]);
    for (size_t j = i + 1; j < roadInfo_1.size(); ++j)
    {
      Junction const finish(roadInfo_1[j].m_points[0]);
      vector<Junction> route;
      TEST_EQUAL(IRouter::NoError, router.CalculateRoute(start, finish, route), ());
      TEST_EQUAL(IRouter::NoError, router.CalculateRoute(finish, start, route), ());
    }
  }

  // Check if there is route between points in the second connected component.
  for (size_t i = 0; i < roadInfo_2.size(); ++i)
  {
    Junction const start(roadInfo_2[i].m_points[0]);
    for (size_t j = i + 1; j < roadInfo_2.size(); ++j)
    {
      Junction const finish(roadInfo_2[j].m_points[0]);
      vector<Junction> route;
      TEST_EQUAL(IRouter::NoError, router.CalculateRoute(start, finish, route), ());
      TEST_EQUAL(IRouter::NoError, router.CalculateRoute(finish, start, route), ());
    }
  }
}

UNIT_TEST(AStarRouter_SimpleGraph_PickTheFasterRoad1)
{
  classificator::Load();

  AStarRouter router([](m2::PointD const & /*point*/)
                     {
                       return "Dummy_map.mwm";
                     });
  {
    unique_ptr<RoadGraphMockSource> graph(new RoadGraphMockSource());

    AddRoad(*graph, 5.0, {m2::PointD(2,1), m2::PointD(2,2), m2::PointD(2,3)});
    AddRoad(*graph, 5.0, {m2::PointD(10,1), m2::PointD(10,2), m2::PointD(10,3)});

    AddRoad(*graph, 5.0, {m2::PointD(2,3), m2::PointD(4,3), m2::PointD(6,3), m2::PointD(8,3), m2::PointD(10,3)});
    AddRoad(*graph, 3.0, {m2::PointD(2,2), m2::PointD(6,2), m2::PointD(10,2)});
    AddRoad(*graph, 4.0, {m2::PointD(2,1), m2::PointD(10,1)});

    router.SetRoadGraph(move(graph));
  }

  // path1 = 1/5 + 8/5 + 1/5 = 2
  // path2 = 8/3 = 2.666(6)
  // path3 = 1/5 + 8/4 + 1/5 = 2.4

  vector<Junction> route;
  TEST_EQUAL(IRouter::NoError, router.CalculateRoute(m2::PointD(2,2), m2::PointD(10,2), route), ());
  TEST_EQUAL(route, vector<Junction>({m2::PointD(2,2), m2::PointD(2,3), m2::PointD(4,3), m2::PointD(6,3),
                                      m2::PointD(8,3), m2::PointD(10,3), m2::PointD(10,2)}), ());
}

UNIT_TEST(AStarRouter_SimpleGraph_PickTheFasterRoad2)
{
  classificator::Load();

  AStarRouter router([](m2::PointD const & /*point*/)
                     {
                       return "Dummy_map.mwm";
                     });
  {
    unique_ptr<RoadGraphMockSource> graph(new RoadGraphMockSource());

    AddRoad(*graph, 5.0, {m2::PointD(2,1), m2::PointD(2,2), m2::PointD(2,3)});
    AddRoad(*graph, 5.0, {m2::PointD(10,1), m2::PointD(10,2), m2::PointD(10,3)});

    AddRoad(*graph, 5.0, {m2::PointD(2,3), m2::PointD(4,3), m2::PointD(6,3), m2::PointD(8,3), m2::PointD(10,3)});
    AddRoad(*graph, 4.1, {m2::PointD(2,2), m2::PointD(6,2), m2::PointD(10,2)});
    AddRoad(*graph, 4.4, {m2::PointD(2,1), m2::PointD(10,1)});

    router.SetRoadGraph(move(graph));
  }

  // path1 = 1/5 + 8/5 + 1/5 = 2
  // path2 = 8/4.1 = 1.95
  // path3 = 1/5 + 8/4.4 + 1/5 = 2.2

  vector<Junction> route;
  TEST_EQUAL(IRouter::NoError, router.CalculateRoute(m2::PointD(2,2), m2::PointD(10,2), route), ());
  TEST_EQUAL(route, vector<Junction>({m2::PointD(2,2), m2::PointD(6,2), m2::PointD(10,2)}), ());
}

UNIT_TEST(AStarRouter_SimpleGraph_PickTheFasterRoad3)
{
  classificator::Load();

  AStarRouter router([](m2::PointD const & /*point*/)
                     {
                       return "Dummy_map.mwm";
                     });
  {
    unique_ptr<RoadGraphMockSource> graph(new RoadGraphMockSource());

    AddRoad(*graph, 5.0, {m2::PointD(2,1), m2::PointD(2,2), m2::PointD(2,3)});
    AddRoad(*graph, 5.0, {m2::PointD(10,1), m2::PointD(10,2), m2::PointD(10,3)});

    AddRoad(*graph, 4.8, {m2::PointD(2,3), m2::PointD(4,3), m2::PointD(6,3), m2::PointD(8,3), m2::PointD(10,3)});
    AddRoad(*graph, 3.9, {m2::PointD(2,2), m2::PointD(6,2), m2::PointD(10,2)});
    AddRoad(*graph, 4.9, {m2::PointD(2,1), m2::PointD(10,1)});

    router.SetRoadGraph(move(graph));
  }

  // path1 = 1/5 + 8/4.8 + 1/5 = 2.04
  // path2 = 8/3.9 = 2.05
  // path3 = 1/5 + 8/4.9 + 1/5 = 2.03

  vector<Junction> route;
  TEST_EQUAL(IRouter::NoError, router.CalculateRoute(m2::PointD(2,2), m2::PointD(10,2), route), ());
  TEST_EQUAL(route, vector<Junction>({m2::PointD(2,2), m2::PointD(2,1), m2::PointD(10,1), m2::PointD(10,2)}), ());
}
