#include "testing/testing.hpp"

#include "routing/routing_tests/road_graph_builder.hpp"
#include "routing/routing_tests/features_road_graph_test.hpp"

#include "routing/nearest_road_pos_finder.hpp"

using namespace routing;
using namespace routing_test;

void TestNearestOnMock1(m2::PointD const & point, size_t const candidatesCount,
                     vector<RoadPos> const & expected)
{
  unique_ptr<RoadGraphMockSource> graph(new RoadGraphMockSource());
  InitRoadGraphMockSourceWithTest1(*graph);
  NearestRoadPosFinder finder(point, m2::PointD::Zero(), *graph.get());
  for (size_t i = 0; i < 4; ++i)
    finder.AddInformationSource(i);
  vector<RoadPos> result;
  TEST(finder.HasCandidates(), ());
  finder.MakeResult(result, candidatesCount);

  for (RoadPos const & pos : expected)
    TEST(find(result.begin(), result.end(), pos) != result.end(), ());
}

UNIT_TEST(StarterPosAtBorder_Mock1Graph)
{
  RoadPos pos1(0, true /* forward */, 0, m2::PointD(0, 0));
  RoadPos pos2(0, false /* backward */, 0, m2::PointD(0, 0));
  vector<RoadPos> expected = {pos1, pos2};
  TestNearestOnMock1(m2::PointD(0, 0), 2, expected);
}

UNIT_TEST(MiddleEdgeTest_Mock1Graph)
{
  RoadPos pos1(0, false /* backward */, 0, m2::PointD(3, 0));
  RoadPos pos2(0, true /* forward */, 0, m2::PointD(3, 0));
  vector<RoadPos> expected = {pos1, pos2};
  TestNearestOnMock1(m2::PointD(3, 3), 2, expected);
}

UNIT_TEST(MiddleSegmentTest_Mock1Graph)
{
  RoadPos pos1(0, false /* backward */, 2, m2::PointD(13, 0));
  RoadPos pos2(0, true /* forward */, 2, m2::PointD(13, 0));
  RoadPos pos3(3, false /* backward */, 1, m2::PointD(15, 5));
  RoadPos pos4(3, true /* forward */, 1, m2::PointD(15, 5));
  vector<RoadPos> expected = {pos1, pos2, pos3, pos4};
  TestNearestOnMock1(m2::PointD(13, 3), 4, expected);
}
