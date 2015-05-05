#include "testing/testing.hpp"

#include "routing/routing_tests/road_graph_builder.hpp"

#include "routing/nearest_edge_finder.hpp"

using namespace routing;
using namespace routing_test;

void TestNearestOnMock1(m2::PointD const & point, size_t const candidatesCount,
                     vector<pair<Edge, m2::PointD>> const & expected)
{
  unique_ptr<RoadGraphMockSource> graph(new RoadGraphMockSource());
  InitRoadGraphMockSourceWithTest1(*graph);

  NearestEdgeFinder finder(point, *graph.get());
  for (size_t i = 0; i < graph->GetRoadCount(); ++i)
    finder.AddInformationSource(i);

  vector<pair<Edge, m2::PointD>> result;
  TEST(finder.HasCandidates(), ());
  finder.MakeResult(result, candidatesCount);

  TEST_EQUAL(result, expected, ());
}

UNIT_TEST(StarterPosAtBorder_Mock1Graph)
{
  vector<pair<Edge, m2::PointD>> const expected =
  {
    make_pair(Edge(0, true /* forward */, 0, m2::PointD(0, 0), m2::PointD(5, 0)), m2::PointD(0, 0))
  };
  TestNearestOnMock1(m2::PointD(0, 0), 1, expected);
}

UNIT_TEST(MiddleEdgeTest_Mock1Graph)
{
  vector<pair<Edge, m2::PointD>> const expected =
  {
    make_pair(Edge(0, true /* forward */, 0, m2::PointD(0, 0), m2::PointD(5, 0)), m2::PointD(3, 0))
  };
  TestNearestOnMock1(m2::PointD(3, 3), 1, expected);
}

UNIT_TEST(MiddleSegmentTest_Mock1Graph)
{
  vector<pair<Edge, m2::PointD>> const expected =
  {
    make_pair(Edge(0, true /* forward */, 2, m2::PointD(10, 0), m2::PointD(15, 0)), m2::PointD(12.5, 0)),
    make_pair(Edge(1, true /* forward */, 2, m2::PointD(10, 0), m2::PointD(10, 5)), m2::PointD(10, 2.5))
  };
  TestNearestOnMock1(m2::PointD(12.5, 2.5), 2, expected);
}
