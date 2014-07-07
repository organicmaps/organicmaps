#include "../../testing/testing.hpp"

#include "road_graph_builder.hpp"

#include "../../base/logging.hpp"


using namespace routing;
using namespace routing_test;


namespace
{

class EqualPos
{
  RoadPos m_pos;
  double m_distance;
public:
  EqualPos(RoadPos const & pos, double d) : m_pos(pos), m_distance(d) {}
  bool operator() (PossibleTurn const & r) const { return (r.m_pos == m_pos && r.m_metersCovered == m_distance); }
};

bool test_result(IRoadGraph::TurnsVectorT const & vec, RoadPos const & pos, double d)
{
  return find_if(vec.begin(), vec.end(), EqualPos(pos, d)) != vec.end();
}

}


UNIT_TEST(RG_Builder_Smoke)
{
  RoadGraphMockSource src;

  {
    RoadInfo ri;
    ri.m_bothSides = true;
    ri.m_speedMS = 40;
    ri.m_points.push_back(m2::PointD(0, 0));
    ri.m_points.push_back(m2::PointD(5, 0));
    ri.m_points.push_back(m2::PointD(10, 0));
    ri.m_points.push_back(m2::PointD(15, 0));
    ri.m_points.push_back(m2::PointD(20, 0));
    src.AddRoad(ri);
  }

  {
    RoadInfo ri;
    ri.m_bothSides = true;
    ri.m_speedMS = 40;
    ri.m_points.push_back(m2::PointD(10, -10));
    ri.m_points.push_back(m2::PointD(10, -5));
    ri.m_points.push_back(m2::PointD(10, 0));
    ri.m_points.push_back(m2::PointD(10, 5));
    ri.m_points.push_back(m2::PointD(10, 10));
    src.AddRoad(ri);
  }

  {
    RoadInfo ri;
    ri.m_bothSides = true;
    ri.m_speedMS = 40;
    ri.m_points.push_back(m2::PointD(15, -5));
    ri.m_points.push_back(m2::PointD(15, 0));
    src.AddRoad(ri);
  }

  {
    RoadInfo ri;
    ri.m_bothSides = true;
    ri.m_speedMS = 40;
    ri.m_points.push_back(m2::PointD(20, 0));
    ri.m_points.push_back(m2::PointD(25, 5));
    ri.m_points.push_back(m2::PointD(15, 5));
    ri.m_points.push_back(m2::PointD(20, 0));
    src.AddRoad(ri);
  }

  {
    IRoadGraph::TurnsVectorT vec;
    src.GetPossibleTurns(RoadPos(0, true, 1), vec);
    TEST_EQUAL(vec.size(), 1, ());
    TEST_EQUAL(vec[0].m_pos, RoadPos(0, true, 0), ());
  }

  {
    IRoadGraph::TurnsVectorT vec;
    src.GetPossibleTurns(RoadPos(0, false, 1), vec);
    TEST_EQUAL(vec.size(), 7, ());
    TEST(test_result(vec, RoadPos(0, false, 2), 5), ());
    TEST(test_result(vec, RoadPos(0, false, 3), 10), ());
    TEST(test_result(vec, RoadPos(1, true, 1), 5), ());
    TEST(test_result(vec, RoadPos(1, false, 2), 5), ());
    TEST(test_result(vec, RoadPos(2, true, 0), 10), ());
    TEST(test_result(vec, RoadPos(3, false, 0), 15), ());
    TEST(test_result(vec, RoadPos(3, true, 2), 15), ());
  }

  {
    IRoadGraph::TurnsVectorT vec;
    src.GetPossibleTurns(RoadPos(1, true, 0), vec);
    TEST_EQUAL(vec.size(), 0, ());
  }

  {
    IRoadGraph::TurnsVectorT vec;
    src.GetPossibleTurns(RoadPos(1, false, 0), vec);
    TEST_EQUAL(vec.size(), 5, ());
    TEST(test_result(vec, RoadPos(1, false, 1), 5), ());
    TEST(test_result(vec, RoadPos(1, false, 2), 10), ());
    TEST(test_result(vec, RoadPos(1, false, 3), 15), ());
    TEST(test_result(vec, RoadPos(0, true, 1), 10), ());
    TEST(test_result(vec, RoadPos(0, false, 2), 10), ());
  }

  RoadInfo ri0;
  ri0.m_bothSides = true;
  ri0.m_speedMS = 40;
  ri0.m_points.push_back(m2::PointD(0, 0));
  ri0.m_points.push_back(m2::PointD(10, 0));
  ri0.m_points.push_back(m2::PointD(25, 0));
  ri0.m_points.push_back(m2::PointD(35, 0));
  ri0.m_points.push_back(m2::PointD(70, 0));
  ri0.m_points.push_back(m2::PointD(80, 0));

  RoadInfo ri1;
  ri1.m_bothSides = false;
  ri1.m_speedMS = 20;
  ri1.m_points.push_back(m2::PointD(0, 0));
  ri1.m_points.push_back(m2::PointD(5, 10));
  ri1.m_points.push_back(m2::PointD(5, 40));

  RoadInfo ri2;
  ri2.m_bothSides = false;
  ri2.m_speedMS = 20;
  ri2.m_points.push_back(m2::PointD(12, 25));
  ri2.m_points.push_back(m2::PointD(10, 10));
  ri2.m_points.push_back(m2::PointD(10, 0));

  RoadInfo ri3;
  ri3.m_bothSides = true;
  ri3.m_speedMS = 30;
  ri3.m_points.push_back(m2::PointD(5, 10));
  ri3.m_points.push_back(m2::PointD(10, 10));
  ri3.m_points.push_back(m2::PointD(70, 10));
  ri3.m_points.push_back(m2::PointD(80, 10));

  RoadInfo ri4;
  ri4.m_bothSides = true;
  ri4.m_speedMS = 20;
  ri4.m_points.push_back(m2::PointD(25, 0));
  ri4.m_points.push_back(m2::PointD(27, 25));

  RoadInfo ri5;
  ri5.m_bothSides = true;
  ri5.m_speedMS = 30;
  ri5.m_points.push_back(m2::PointD(35, 0));
  ri5.m_points.push_back(m2::PointD(37, 30));
  ri5.m_points.push_back(m2::PointD(70, 30));
  ri5.m_points.push_back(m2::PointD(80, 30));

  RoadInfo ri6;
  ri6.m_bothSides = true;
  ri6.m_speedMS = 20;
  ri6.m_points.push_back(m2::PointD(70, 0));
  ri6.m_points.push_back(m2::PointD(70, 10));
  ri6.m_points.push_back(m2::PointD(70, 30));

  RoadInfo ri7;
  ri7.m_bothSides = true;
  ri7.m_speedMS = 20;
  ri7.m_points.push_back(m2::PointD(39, 55));
  ri7.m_points.push_back(m2::PointD(80, 55));

  RoadInfo ri8;
  ri8.m_bothSides = false;
  ri8.m_speedMS = 30;
  ri8.m_points.push_back(m2::PointD(5, 40));
  ri8.m_points.push_back(m2::PointD(18, 55));
  ri8.m_points.push_back(m2::PointD(39, 55));
  ri8.m_points.push_back(m2::PointD(37, 30));
  ri8.m_points.push_back(m2::PointD(27, 25));
  ri8.m_points.push_back(m2::PointD(12, 25));
  ri8.m_points.push_back(m2::PointD(5, 40));

  RoadGraphMockSource graph;
  graph.AddRoad(ri0);
  graph.AddRoad(ri1);
  graph.AddRoad(ri2);
  graph.AddRoad(ri3);
  graph.AddRoad(ri4);
  graph.AddRoad(ri5);
  graph.AddRoad(ri6);
  graph.AddRoad(ri7);
  graph.AddRoad(ri8);

  {
    IRoadGraph::TurnsVectorT vec;
    graph.GetPossibleTurns(RoadPos(0, false, 0), vec);
    TEST_EQUAL(vec.size(), 8, ());
    TEST(test_result(vec, RoadPos(0, false, 1), 10), ());
    TEST(test_result(vec, RoadPos(0, false, 2), 25), ());
    TEST(test_result(vec, RoadPos(0, false, 3), 35), ());
    TEST(test_result(vec, RoadPos(0, false, 4), 70), ());
    TEST(test_result(vec, RoadPos(2, true, 1), 10), ());
    TEST(test_result(vec, RoadPos(5, false, 0), 35), ());
    TEST(test_result(vec, RoadPos(6, false, 0), 70), ());
    TEST(test_result(vec, RoadPos(4, false, 0), 25), ());
  }

  {
    IRoadGraph::TurnsVectorT vec;
    graph.GetPossibleTurns(RoadPos(8, true, 0), vec);
    double const d = m2::PointD(18, 55).Length(m2::PointD(5, 40));
    TEST_EQUAL(vec.size(), 2, ());
    TEST(test_result(vec, RoadPos(1, true, 1), d), ());
    TEST(test_result(vec, RoadPos(8, true, 5), d), ());
  }

  {
    IRoadGraph::TurnsVectorT vec;
    graph.GetPossibleTurns(RoadPos(2, true, 1), vec);
    TEST_EQUAL(vec.size(), 4, ());
    TEST(test_result(vec, RoadPos(3, true, 0), 10), ());
    TEST(test_result(vec, RoadPos(3, false, 1), 10), ());
    TEST(test_result(vec, RoadPos(2, true, 0), 10), ());
    TEST(test_result(vec, RoadPos(8, true, 4), 10 + m2::PointD(10, 10).Length(m2::PointD(12, 25))), ());
  }

  {
    IRoadGraph::TurnsVectorT vec;
    graph.GetPossibleTurns(RoadPos(3, false, 0), vec);
    TEST_EQUAL(vec.size(), 5, ());
    TEST(test_result(vec, RoadPos(3, false, 1), 5), ());
    TEST(test_result(vec, RoadPos(3, false, 2), 65), ());
    TEST(test_result(vec, RoadPos(2, true, 0), 5), ());
    TEST(test_result(vec, RoadPos(6, true, 0), 65), ());
    TEST(test_result(vec, RoadPos(6, false, 1), 65), ());
  }

  {
    IRoadGraph::TurnsVectorT vec;
    graph.GetPossibleTurns(RoadPos(7, false, 0), vec);
    TEST_EQUAL(vec.size(), 0, ());
  }

  {
    IRoadGraph::TurnsVectorT vec;
    graph.GetPossibleTurns(RoadPos(7, true, 0), vec);
    TEST_EQUAL(vec.size(), 1, ());
    TEST(test_result(vec, RoadPos(8, true, 1), 41), ());
  }

  {
    IRoadGraph::TurnsVectorT vec;
    graph.GetPossibleTurns(RoadPos(8, true, 3), vec);
    double d = m2::PointD(27, 25).Length(m2::PointD(37, 30));
    TEST_EQUAL(vec.size(), 8, ());
    TEST(test_result(vec, RoadPos(8, true, 2), d), ());
    TEST(test_result(vec, RoadPos(5, true, 0), d), ());
    TEST(test_result(vec, RoadPos(5, false, 1), d), ());
    d += m2::PointD(37, 30).Length(m2::PointD(39, 55));
    TEST(test_result(vec, RoadPos(7, false, 0), d), ());;
    TEST(test_result(vec, RoadPos(8, true, 1), d), ());
    d += m2::PointD(18, 55).Length(m2::PointD(39, 55));
    TEST(test_result(vec, RoadPos(8, true, 0), d), ());
    d += m2::PointD(5, 40).Length(m2::PointD(18, 55));
    TEST(test_result(vec, RoadPos(1, true, 1), d), ());
    TEST(test_result(vec, RoadPos(8, true, 5), d), ());
  }
}
