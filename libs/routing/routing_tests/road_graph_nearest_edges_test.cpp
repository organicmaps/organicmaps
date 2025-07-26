#include "testing/testing.hpp"

#include "routing/road_graph.hpp"
#include "routing/routing_tests/road_graph_builder.hpp"

#include "geometry/point_with_altitude.hpp"

#include <algorithm>
#include <utility>

namespace routing_test
{

using namespace routing;
using namespace std;

UNIT_TEST(RoadGraph_NearestEdges)
{
  //        ^  1st road
  //        o
  //        |
  //        |
  //        o
  //        |
  //        |       0th road
  //  o--o--x--o--o >
  //        |
  //        |
  //        o
  //        |
  //        |
  //        o
  //
  // Two roads are intersecting in (0, 0).
  RoadGraphMockSource graph;
  {
    IRoadGraph::RoadInfo ri0;
    ri0.m_junctions.push_back(geometry::MakePointWithAltitudeForTesting(m2::PointD(-2, 0)));
    ri0.m_junctions.push_back(geometry::MakePointWithAltitudeForTesting(m2::PointD(-1, 0)));
    ri0.m_junctions.push_back(geometry::MakePointWithAltitudeForTesting(m2::PointD(0, 0)));
    ri0.m_junctions.push_back(geometry::MakePointWithAltitudeForTesting(m2::PointD(1, 0)));
    ri0.m_junctions.push_back(geometry::MakePointWithAltitudeForTesting(m2::PointD(2, 0)));
    ri0.m_speedKMPH = 5;
    ri0.m_bidirectional = true;

    IRoadGraph::RoadInfo ri1;
    ri1.m_junctions.push_back(geometry::MakePointWithAltitudeForTesting(m2::PointD(0, -2)));
    ri1.m_junctions.push_back(geometry::MakePointWithAltitudeForTesting(m2::PointD(0, -1)));
    ri1.m_junctions.push_back(geometry::MakePointWithAltitudeForTesting(m2::PointD(0, 0)));
    ri1.m_junctions.push_back(geometry::MakePointWithAltitudeForTesting(m2::PointD(0, 1)));
    ri1.m_junctions.push_back(geometry::MakePointWithAltitudeForTesting(m2::PointD(0, 2)));
    ri1.m_speedKMPH = 5;
    ri1.m_bidirectional = true;

    graph.AddRoad(std::move(ri0));
    graph.AddRoad(std::move(ri1));
  }

  // We are standing at x junction.
  geometry::PointWithAltitude const crossPos = geometry::MakePointWithAltitudeForTesting(m2::PointD(0, 0));

  // Expected outgoing edges.
  IRoadGraph::EdgeListT expectedOutgoing = {
      Edge::MakeReal(MakeTestFeatureID(0) /* first road */, false /* forward */, 1 /* segId */,
                     geometry::MakePointWithAltitudeForTesting(m2::PointD(0, 0)),
                     geometry::MakePointWithAltitudeForTesting(m2::PointD(-1, 0))),
      Edge::MakeReal(MakeTestFeatureID(0) /* first road */, true /* forward */, 2 /* segId */,
                     geometry::MakePointWithAltitudeForTesting(m2::PointD(0, 0)),
                     geometry::MakePointWithAltitudeForTesting(m2::PointD(1, 0))),
      Edge::MakeReal(MakeTestFeatureID(1) /* second road */, false /* forward */, 1 /* segId */,
                     geometry::MakePointWithAltitudeForTesting(m2::PointD(0, 0)),
                     geometry::MakePointWithAltitudeForTesting(m2::PointD(0, -1))),
      Edge::MakeReal(MakeTestFeatureID(1) /* second road */, true /* forward */, 2 /* segId */,
                     geometry::MakePointWithAltitudeForTesting(m2::PointD(0, 0)),
                     geometry::MakePointWithAltitudeForTesting(m2::PointD(0, 1))),
  };
  sort(expectedOutgoing.begin(), expectedOutgoing.end());

  // Expected ingoing edges.
  IRoadGraph::EdgeListT expectedIngoing = {
      Edge::MakeReal(MakeTestFeatureID(0) /* first road */, true /* forward */, 1 /* segId */,
                     geometry::MakePointWithAltitudeForTesting(m2::PointD(-1, 0)),
                     geometry::MakePointWithAltitudeForTesting(m2::PointD(0, 0))),
      Edge::MakeReal(MakeTestFeatureID(0) /* first road */, false /* forward */, 2 /* segId */,
                     geometry::MakePointWithAltitudeForTesting(m2::PointD(1, 0)),
                     geometry::MakePointWithAltitudeForTesting(m2::PointD(0, 0))),
      Edge::MakeReal(MakeTestFeatureID(1) /* second road */, true /* forward */, 1 /* segId */,
                     geometry::MakePointWithAltitudeForTesting(m2::PointD(0, -1)),
                     geometry::MakePointWithAltitudeForTesting(m2::PointD(0, 0))),
      Edge::MakeReal(MakeTestFeatureID(1) /* second road */, false /* forward */, 2 /* segId */,
                     geometry::MakePointWithAltitudeForTesting(m2::PointD(0, 1)),
                     geometry::MakePointWithAltitudeForTesting(m2::PointD(0, 0))),
  };
  sort(expectedIngoing.begin(), expectedIngoing.end());

  // Check outgoing edges.
  IRoadGraph::EdgeListT actualOutgoing;
  graph.GetOutgoingEdges(crossPos, actualOutgoing);
  sort(actualOutgoing.begin(), actualOutgoing.end());
  TEST_EQUAL(expectedOutgoing, actualOutgoing, ());

  // Check ingoing edges.
  IRoadGraph::EdgeListT actualIngoing;
  graph.GetIngoingEdges(crossPos, actualIngoing);
  sort(actualIngoing.begin(), actualIngoing.end());
  TEST_EQUAL(expectedIngoing, actualIngoing, ());
}

}  // namespace routing_test
