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


UNIT_TEST(RG_Builder_Test1)
{
  RoadGraphMockSource src;
  InitRoadGraphMockSourceWithTest1(src);

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

}

UNIT_TEST(RG_Builder_Test2)
{
  RoadGraphMockSource graph;
  InitRoadGraphMockSourceWithTest2(graph);
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
