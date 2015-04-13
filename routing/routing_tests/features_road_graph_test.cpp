#include "testing/testing.hpp"

#include "routing/routing_tests/features_road_graph_test.hpp"

#include "routing/route.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/feature.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/logging.hpp"


using namespace routing;

namespace routing_test
{

class EqualPos
{
  RoadPos m_pos;
  double m_distance;
public:
  EqualPos(RoadPos const & pos, double d) : m_pos(pos), m_distance(d) {}
  bool operator() (PossibleTurn const & r) const
  {
    return r.m_pos == m_pos;
  }
};

bool TestResult(IRoadGraph::TurnsVectorT const & vec, RoadPos const & pos, double d)
{
  return find_if(vec.begin(), vec.end(), EqualPos(pos, d)) != vec.end();
}



void Name2IdMapping::operator()(FeatureType const & ft)
{
  if (!ftypes::IsStreetChecker::Instance()(ft))
    return;

  string name;
  VERIFY(ft.GetName(0, name), ());

  m_name2Id[name] = ft.GetID().m_offset;
  m_id2Name[ft.GetID().m_offset] = name;
}

uint32_t Name2IdMapping::GetId(string const & name)
{
  ASSERT(m_name2Id.find(name) != m_name2Id.end(), ());
  return m_name2Id[name];
}

string const & Name2IdMapping::GetName(uint32_t id)
{
  ASSERT(m_id2Name.find(id) != m_id2Name.end(), ());
  return m_id2Name[id];
}


FeatureRoadGraphTester::FeatureRoadGraphTester(string const & name)
{
  classificator::Load();

  pair<MwmSet::MwmLock, bool> p = m_index.RegisterMap(name);
  if (!p.second)
  {
    LOG(LERROR, ("MWM file not found"));
    return;
  }

  m_graph.reset(new FeaturesRoadGraph(&m_index, 0));

  m_index.ForEachInRect(m_mapping, MercatorBounds::FullRect(), m_graph->GetStreetReadScale());
}

void FeatureRoadGraphTester::FeatureID2Name(routing::RoadPos & pos)
{
  string const & name = m_mapping.GetName(pos.GetFeatureId());
  int id = 0;
  VERIFY(strings::to_int(name, id), (name));

  pos = RoadPos(static_cast<uint32_t>(id), pos.IsForward(), pos.GetSegId());
}

void FeatureRoadGraphTester::FeatureID2Name(IRoadGraph::TurnsVectorT & vec)
{
  for (size_t i = 0; i < vec.size(); ++i)
    FeatureID2Name(vec[i].m_pos);
}

void FeatureRoadGraphTester::FeatureID2Name(vector<routing::RoadPos> & vec)
{
  for (size_t i = 0; i < vec.size(); ++i)
    FeatureID2Name(vec[i]);
}

void FeatureRoadGraphTester::Name2FeatureID(vector<routing::RoadPos> & vec)
{
  for (size_t i = 0; i < vec.size(); ++i)
    vec[i] = RoadPos(m_mapping.GetId(strings::to_string(vec[i].GetFeatureId())), vec[i].IsForward(),
                     vec[i].GetSegId());
}

void FeatureRoadGraphTester::GetPossibleTurns(RoadPos const & pos, IRoadGraph::TurnsVectorT & vec)
{
  m_graph->GetNearestTurns(RoadPos(m_mapping.GetId(strings::to_string(pos.GetFeatureId())),
                                   pos.IsForward(), pos.GetSegId()),
                           vec);
  FeatureID2Name(vec);
}

template <size_t N>
void FeatureRoadGraphTester::ReconstructPath(routing::RoadPos (&arr)[N], vector<m2::PointD> & vec)
{
  vector<RoadPos> positions(arr, arr + N);
  Name2FeatureID(positions);

  Route route("dummy");
  m_graph->ReconstructPath(positions, route);
  vec.assign(route.GetPoly().Begin(), route.GetPoly().End());
}

}


using namespace routing_test;


UNIT_TEST(FRG_TurnsTest_MWM1)
{
  FeatureRoadGraphTester tester("route_test1.mwm");

  {
    IRoadGraph::TurnsVectorT vec;
    tester.GetPossibleTurns(RoadPos(0, true, 1), vec);
    TEST_EQUAL(vec.size(), 1, ());
    TEST(TestResult(vec, RoadPos(0, true, 0), -1), ());
  }

  {
    IRoadGraph::TurnsVectorT vec;
    tester.GetPossibleTurns(RoadPos(0, false, 1), vec);
    TEST_EQUAL(vec.size(), 7, ());
    TEST(TestResult(vec, RoadPos(0, false, 2), -1), ());
    TEST(TestResult(vec, RoadPos(0, false, 3), -1), ());
    TEST(TestResult(vec, RoadPos(1, true, 1), -1), ());
    TEST(TestResult(vec, RoadPos(1, false, 2), -1), ());
    TEST(TestResult(vec, RoadPos(2, true, 0), -1), ());
    TEST(TestResult(vec, RoadPos(3, false, 0), -1), ());
    TEST(TestResult(vec, RoadPos(3, true, 2), -1), ());
  }

  {
    IRoadGraph::TurnsVectorT vec;
    tester.GetPossibleTurns(RoadPos(1, true, 0), vec);
    TEST_EQUAL(vec.size(), 0, ());
  }

  {
    IRoadGraph::TurnsVectorT vec;
    tester.GetPossibleTurns(RoadPos(1, false, 0), vec);
    TEST_EQUAL(vec.size(), 3, ());
    TEST(TestResult(vec, RoadPos(1, false, 2), 10), ());
    TEST(TestResult(vec, RoadPos(0, true, 1), 10), ());
    TEST(TestResult(vec, RoadPos(0, false, 2), 10), ());
  }
}

UNIT_TEST(FRG_TurnsTest_MWM2)
{
  FeatureRoadGraphTester tester("route_test2.mwm");

  {
    IRoadGraph::TurnsVectorT vec;
    tester.GetPossibleTurns(RoadPos(0, false, 0), vec);
    TEST_EQUAL(vec.size(), 8, ());
    TEST(TestResult(vec, RoadPos(0, false, 1), -1), ());
    TEST(TestResult(vec, RoadPos(0, false, 2), -1), ());
    TEST(TestResult(vec, RoadPos(0, false, 3), -1), ());
    TEST(TestResult(vec, RoadPos(0, false, 4), -1), ());
    TEST(TestResult(vec, RoadPos(2, true, 1), -1), ());
    TEST(TestResult(vec, RoadPos(5, false, 0), -1), ());
    TEST(TestResult(vec, RoadPos(6, false, 0), -1), ());
    TEST(TestResult(vec, RoadPos(4, false, 0), -1), ());
  }

  {
    IRoadGraph::TurnsVectorT vec;
    tester.GetPossibleTurns(RoadPos(8, true, 0), vec);
    TEST_EQUAL(vec.size(), 2, ());
    TEST(TestResult(vec, RoadPos(1, true, 1), -1), ());
    TEST(TestResult(vec, RoadPos(8, true, 5), -1), ());
  }

  {
    IRoadGraph::TurnsVectorT vec;
    tester.GetPossibleTurns(RoadPos(2, true, 1), vec);
    TEST_EQUAL(vec.size(), 4, ());
    TEST(TestResult(vec, RoadPos(3, true, 0), -1), ());
    TEST(TestResult(vec, RoadPos(3, false, 1), -1), ());
    TEST(TestResult(vec, RoadPos(2, true, 0), -1), ());
    TEST(TestResult(vec, RoadPos(8, true, 4), -1), ());
  }

  {
    IRoadGraph::TurnsVectorT vec;
    tester.GetPossibleTurns(RoadPos(3, false, 0), vec);
    TEST_EQUAL(vec.size(), 5, ());
    TEST(TestResult(vec, RoadPos(3, false, 1), -1), ());
    TEST(TestResult(vec, RoadPos(3, false, 2), -1), ());
    TEST(TestResult(vec, RoadPos(2, true, 0), -1), ());
    TEST(TestResult(vec, RoadPos(6, true, 0), -1), ());
    TEST(TestResult(vec, RoadPos(6, false, 1), -1), ());
  }

  {
    IRoadGraph::TurnsVectorT vec;
    tester.GetPossibleTurns(RoadPos(7, false, 0), vec);
    TEST_EQUAL(vec.size(), 0, ());
  }

  {
    IRoadGraph::TurnsVectorT vec;
    tester.GetPossibleTurns(RoadPos(7, true, 0), vec);
    TEST_EQUAL(vec.size(), 1, ());
    TEST(TestResult(vec, RoadPos(8, true, 1), -1), ());
  }

  {
    IRoadGraph::TurnsVectorT vec;
    tester.GetPossibleTurns(RoadPos(8, true, 3), vec);
    TEST_EQUAL(vec.size(), 7, ());
    TEST(TestResult(vec, RoadPos(8, true, 2), -1), ());
    TEST(TestResult(vec, RoadPos(5, true, 0), -1), ());
    TEST(TestResult(vec, RoadPos(5, false, 1), -1), ());
    TEST(TestResult(vec, RoadPos(7, false, 0), -1), ());
    TEST(TestResult(vec, RoadPos(8, true, 1), -1), ());
    TEST(TestResult(vec, RoadPos(1, true, 1), -1), ());
    TEST(TestResult(vec, RoadPos(8, true, 5), -1), ());
  }
}

UNIT_TEST(FRG_ReconstructTest_MWM2)
{
  // Uncomment to see debug log.
  //my::g_LogLevel = LDEBUG;

  FeatureRoadGraphTester tester("route_test2.mwm");

  {
    RoadPos arr[] = {
      RoadPos(8, true, 4),
      RoadPos(2, true, 0),
      RoadPos(3, true, 2)
    };

    vector<m2::PointD> vec;
    tester.ReconstructPath(arr, vec);
    TEST_EQUAL(vec.size(), 3, ());
  }
}
