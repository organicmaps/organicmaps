#include "testing/testing.hpp"

#include "routing/routing_tests/road_graph_builder.hpp"

#include "routing/nearest_edge_finder.hpp"
#include "routing/road_graph.hpp"

#include "routing_common/maxspeed_conversion.hpp"

#include "geometry/point_with_altitude.hpp"

#include "base/checked_cast.hpp"

namespace nearest_edge_finder_tests
{
using namespace routing;
using namespace routing_test;
using namespace std;

void TestNearestOnMock1(m2::PointD const & point, size_t const candidatesCount,
                        vector<pair<Edge, geometry::PointWithAltitude>> const & expected)
{
  unique_ptr<RoadGraphMockSource> graph(new RoadGraphMockSource());
  InitRoadGraphMockSourceWithTest1(*graph);

  NearestEdgeFinder finder(point, nullptr /* isEdgeProjGood */);
  for (size_t i = 0; i < graph->GetRoadCount(); ++i)
  {
    FeatureID const featureId = MakeTestFeatureID(base::checked_cast<uint32_t>(i));
    auto const & roadInfo = graph->GetRoadInfo(featureId, {true /* forward */, false /* in city */, Maxspeed()});
    finder.AddInformationSource(IRoadGraph::FullRoadInfo(featureId, roadInfo));
  }

  vector<pair<Edge, geometry::PointWithAltitude>> result;
  TEST(finder.HasCandidates(), ());
  finder.MakeResult(result, candidatesCount);

  TEST_EQUAL(result, expected, ());
}

UNIT_TEST(StarterPosAtBorder_Mock1Graph)
{
  vector<pair<Edge, geometry::PointWithAltitude>> const expected = {
      make_pair(Edge::MakeReal(MakeTestFeatureID(0), true /* forward */, 0,
                               geometry::MakePointWithAltitudeForTesting(m2::PointD(0, 0)),
                               geometry::MakePointWithAltitudeForTesting(m2::PointD(5, 0))),
                geometry::MakePointWithAltitudeForTesting(m2::PointD(0, 0)))};
  TestNearestOnMock1(m2::PointD(0, 0), 1, expected);
}

UNIT_TEST(MiddleEdgeTest_Mock1Graph)
{
  vector<pair<Edge, geometry::PointWithAltitude>> const expected = {
      make_pair(Edge::MakeReal(MakeTestFeatureID(0), true /* forward */, 0,
                               geometry::MakePointWithAltitudeForTesting(m2::PointD(0, 0)),
                               geometry::MakePointWithAltitudeForTesting(m2::PointD(5, 0))),
                geometry::MakePointWithAltitudeForTesting(m2::PointD(3, 0)))};
  TestNearestOnMock1(m2::PointD(3, 3), 1, expected);
}

UNIT_TEST(MiddleSegmentTest_Mock1Graph)
{
  vector<pair<Edge, geometry::PointWithAltitude>> const expected = {
      make_pair(Edge::MakeReal(MakeTestFeatureID(0), true /* forward */, 2,
                               geometry::MakePointWithAltitudeForTesting(m2::PointD(10, 0)),
                               geometry::MakePointWithAltitudeForTesting(m2::PointD(15, 0))),
                geometry::MakePointWithAltitudeForTesting(m2::PointD(12.5, 0))),
      make_pair(Edge::MakeReal(MakeTestFeatureID(0), false /* forward */, 2,
                               geometry::MakePointWithAltitudeForTesting(m2::PointD(15, 0)),
                               geometry::MakePointWithAltitudeForTesting(m2::PointD(10, 0))),
                geometry::MakePointWithAltitudeForTesting(m2::PointD(12.5, 0)))};
  TestNearestOnMock1(m2::PointD(12.5, 2.5), 2, expected);
}
}  // namespace nearest_edge_finder_tests
