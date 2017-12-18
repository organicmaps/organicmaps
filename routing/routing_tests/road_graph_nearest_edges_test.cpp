#include "testing/testing.hpp"

#include "routing/road_graph.hpp"
#include "routing/routing_tests/road_graph_builder.hpp"
#include "std/algorithm.hpp"

namespace routing_test
{

using namespace routing;

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
    ri0.m_junctions.push_back(MakeJunctionForTesting(m2::PointD(-2, 0)));
    ri0.m_junctions.push_back(MakeJunctionForTesting(m2::PointD(-1, 0)));
    ri0.m_junctions.push_back(MakeJunctionForTesting(m2::PointD(0, 0)));
    ri0.m_junctions.push_back(MakeJunctionForTesting(m2::PointD(1, 0)));
    ri0.m_junctions.push_back(MakeJunctionForTesting(m2::PointD(2, 0)));
    ri0.m_speedKMPH = 5;
    ri0.m_bidirectional = true;

    IRoadGraph::RoadInfo ri1;
    ri1.m_junctions.push_back(MakeJunctionForTesting(m2::PointD(0, -2)));
    ri1.m_junctions.push_back(MakeJunctionForTesting(m2::PointD(0, -1)));
    ri1.m_junctions.push_back(MakeJunctionForTesting(m2::PointD(0, 0)));
    ri1.m_junctions.push_back(MakeJunctionForTesting(m2::PointD(0, 1)));
    ri1.m_junctions.push_back(MakeJunctionForTesting(m2::PointD(0, 2)));
    ri1.m_speedKMPH = 5;
    ri1.m_bidirectional = true;

    graph.AddRoad(move(ri0));
    graph.AddRoad(move(ri1));
  }

  // We are standing at x junction.
  Junction const crossPos = MakeJunctionForTesting(m2::PointD(0, 0));

  // Expected outgoing edges.
  IRoadGraph::TEdgeVector expectedOutgoing = {
      Edge::MakeReal(MakeTestFeatureID(0) /* first road */, false /* forward */, 1 /* segId */,
                     MakeJunctionForTesting(m2::PointD(0, 0)),
                     MakeJunctionForTesting(m2::PointD(-1, 0))),
      Edge::MakeReal(MakeTestFeatureID(0) /* first road */, true /* forward */, 2 /* segId */,
                     MakeJunctionForTesting(m2::PointD(0, 0)),
                     MakeJunctionForTesting(m2::PointD(1, 0))),
      Edge::MakeReal(MakeTestFeatureID(1) /* second road */, false /* forward */, 1 /* segId */,
                     MakeJunctionForTesting(m2::PointD(0, 0)),
                     MakeJunctionForTesting(m2::PointD(0, -1))),
      Edge::MakeReal(MakeTestFeatureID(1) /* second road */, true /* forward */, 2 /* segId */,
                     MakeJunctionForTesting(m2::PointD(0, 0)),
                     MakeJunctionForTesting(m2::PointD(0, 1))),
  };
  sort(expectedOutgoing.begin(), expectedOutgoing.end());

  // Expected ingoing edges.
  IRoadGraph::TEdgeVector expectedIngoing = {
      Edge::MakeReal(MakeTestFeatureID(0) /* first road */, true /* forward */, 1 /* segId */,
                     MakeJunctionForTesting(m2::PointD(-1, 0)),
                     MakeJunctionForTesting(m2::PointD(0, 0))),
      Edge::MakeReal(MakeTestFeatureID(0) /* first road */, false /* forward */, 2 /* segId */,
                     MakeJunctionForTesting(m2::PointD(1, 0)),
                     MakeJunctionForTesting(m2::PointD(0, 0))),
      Edge::MakeReal(MakeTestFeatureID(1) /* second road */, true /* forward */, 1 /* segId */,
                     MakeJunctionForTesting(m2::PointD(0, -1)),
                     MakeJunctionForTesting(m2::PointD(0, 0))),
      Edge::MakeReal(MakeTestFeatureID(1) /* second road */, false /* forward */, 2 /* segId */,
                     MakeJunctionForTesting(m2::PointD(0, 1)),
                     MakeJunctionForTesting(m2::PointD(0, 0))),
  };
  sort(expectedIngoing.begin(), expectedIngoing.end());

  // Check outgoing edges.
  IRoadGraph::TEdgeVector actualOutgoing;
  graph.GetOutgoingEdges(crossPos, actualOutgoing);
  sort(actualOutgoing.begin(), actualOutgoing.end());
  TEST_EQUAL(expectedOutgoing, actualOutgoing, ());

  // Check ingoing edges.
  IRoadGraph::TEdgeVector actualIngoing;
  graph.GetIngoingEdges(crossPos, actualIngoing);
  sort(actualIngoing.begin(), actualIngoing.end());
  TEST_EQUAL(expectedIngoing, actualIngoing, ());
}

}  // namespace routing_test
