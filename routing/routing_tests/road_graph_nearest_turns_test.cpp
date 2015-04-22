#include "testing/testing.hpp"

#include "routing/road_graph.hpp"
#include "routing/routing_tests/road_graph_builder.hpp"
#include "std/algorithm.hpp"

namespace routing
{
UNIT_TEST(RoadGraph_NearestTurns)
{
  //           1st road
  //        o
  //        |
  //        |
  //        o
  //        |
  //        |        0th road
  //  o--o--x--o--o
  //        |
  //        |
  //        o
  //        |
  //        |
  //        o
  //
  // Two roads intersecting at (0, 0).
  routing_test::RoadGraphMockSource graph;
  {
    IRoadGraph::RoadInfo ri0;
    ri0.m_points.emplace_back(-2, 0);
    ri0.m_points.emplace_back(-1, 0);
    ri0.m_points.emplace_back(0, 0);
    ri0.m_points.emplace_back(1, 0);
    ri0.m_points.emplace_back(2, 0);
    ri0.m_speedKMPH = 5;
    ri0.m_bidirectional = true;

    IRoadGraph::RoadInfo ri1;
    ri1.m_points.emplace_back(0, -2);
    ri1.m_points.emplace_back(0, -1);
    ri1.m_points.emplace_back(0, 0);
    ri1.m_points.emplace_back(0, 1);
    ri1.m_points.emplace_back(0, 2);
    ri1.m_speedKMPH = 5;
    ri1.m_bidirectional = true;

    graph.AddRoad(move(ri0));
    graph.AddRoad(move(ri1));
  }

  // We are standing at ... x--o ... segment and are looking to the
  // right.
  RoadPos const crossPos(0 /* featureId */, true /* forward */, 2 /* segId */, m2::PointD(0, 0));
  IRoadGraph::RoadPosVectorT expected = {
      // It is possible to get to the RoadPos we are standing at from
      // RoadPos'es on the first road marked with >> and <<.
      //
      //        ...
      //         |
      // ...--o>>x<<o--...
      //         |
      //        ...
      //
      RoadPos(0 /* first road */, true /* forward */, 1 /* segId */, m2::PointD(0, 0)),
      RoadPos(0 /* first road */, false /* forward */, 2 /* segId */, m2::PointD(0, 0)),

      // It is possible to get to the RoadPos we are standing at from
      // RoadPos'es on the second road marked with v and ^.
      //
      //     ...
      //      |
      //      o
      //      v
      //      v
      //  ...-x-...
      //      ^
      //      ^
      //      o
      //      |
      //     ...
      //
      RoadPos(1 /* first road */, true /* forward */, 1 /* segId */, m2::PointD(0, 0)),
      RoadPos(1 /* first road */, false /* forward */, 2 /* segId */, m2::PointD(0, 0)),
  };

  IRoadGraph::TurnsVectorT turns;
  graph.GetNearestTurns(crossPos, turns);

  IRoadGraph::RoadPosVectorT actual;
  for (PossibleTurn const & turn : turns) {
    actual.push_back(turn.m_pos);
    TEST_EQUAL(5, turn.m_speedKMPH, ());
    TEST_EQUAL(0, turn.m_metersCovered, ());
    TEST_EQUAL(0, turn.m_secondsCovered, ());
  }

  sort(expected.begin(), expected.end());
  sort(actual.begin(), actual.end());
  TEST_EQUAL(expected, actual, ());
}
}  // namespace routing
