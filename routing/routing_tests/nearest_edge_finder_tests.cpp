#include "testing/testing.hpp"

#include "routing/routing_tests/road_graph_builder.hpp"

#include "routing/nearest_edge_finder.hpp"

#include "routing_common/maxspeed_conversion.hpp"

#include "base/checked_cast.hpp"

using namespace routing;
using namespace routing_test;

void TestNearestOnMock1(m2::PointD const & point, size_t const candidatesCount,
                        vector<pair<Edge, Junction>> const & expected)
{
  unique_ptr<RoadGraphMockSource> graph(new RoadGraphMockSource());
  InitRoadGraphMockSourceWithTest1(*graph);

  NearestEdgeFinder finder(point);
  for (size_t i = 0; i < graph->GetRoadCount(); ++i)
  {
    FeatureID const featureId = MakeTestFeatureID(base::checked_cast<uint32_t>(i));
    auto const & roadInfo =
        graph->GetRoadInfo(featureId, {true /* forward */, false /* in city */, Maxspeed()});
    finder.AddInformationSource(featureId, roadInfo.m_junctions, roadInfo.m_bidirectional);
  }

  vector<pair<Edge, Junction>> result;
  TEST(finder.HasCandidates(), ());
  finder.MakeResult(result, candidatesCount);

  TEST_EQUAL(result, expected, ());
}

UNIT_TEST(StarterPosAtBorder_Mock1Graph)
{
  vector<pair<Edge, Junction>> const expected = {
      make_pair(Edge::MakeReal(MakeTestFeatureID(0), true /* forward */, 0,
                               MakeJunctionForTesting(m2::PointD(0, 0)),
                               MakeJunctionForTesting(m2::PointD(5, 0))),
                MakeJunctionForTesting(m2::PointD(0, 0)))};
  TestNearestOnMock1(m2::PointD(0, 0), 1, expected);
}

UNIT_TEST(MiddleEdgeTest_Mock1Graph)
{
  vector<pair<Edge, Junction>> const expected = {
      make_pair(Edge::MakeReal(MakeTestFeatureID(0), true /* forward */, 0,
                               MakeJunctionForTesting(m2::PointD(0, 0)),
                               MakeJunctionForTesting(m2::PointD(5, 0))),
                MakeJunctionForTesting(m2::PointD(3, 0)))};
  TestNearestOnMock1(m2::PointD(3, 3), 1, expected);
}

UNIT_TEST(MiddleSegmentTest_Mock1Graph)
{
  vector<pair<Edge, Junction>> const expected = {
      make_pair(Edge::MakeReal(MakeTestFeatureID(0), true /* forward */, 2,
                               MakeJunctionForTesting(m2::PointD(10, 0)),
                               MakeJunctionForTesting(m2::PointD(15, 0))),
                MakeJunctionForTesting(m2::PointD(12.5, 0))),
      make_pair(Edge::MakeReal(MakeTestFeatureID(0), false /* forward */, 2,
                               MakeJunctionForTesting(m2::PointD(15, 0)),
                               MakeJunctionForTesting(m2::PointD(10, 0))),
                MakeJunctionForTesting(m2::PointD(12.5, 0)))};
  TestNearestOnMock1(m2::PointD(12.5, 2.5), 2, expected);
}
