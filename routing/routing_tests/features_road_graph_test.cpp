#include "../../testing/testing.hpp"

#include "../../base/logging.hpp"

#include "../../indexer/classificator_loader.hpp"
#include "../../indexer/index.hpp"
#include "../../indexer/feature.hpp"

#include "../../search/ftypes_matcher.hpp"

#include "../features_road_graph.hpp"

using namespace routing;


namespace
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

class Name2IdMapping
{
  map<string, uint32_t> m_name2Id;
  map<uint32_t, string> m_id2Name;
  ftypes::IsStreetChecker m_checker;

public:
  void operator()(FeatureType const & ft)
  {
    if (!m_checker(ft)) return;

    string name;
    bool hasName = ft.GetName(0, name);
    ASSERT(hasName, ());

    m_name2Id[name] = ft.GetID().m_offset;
    m_id2Name[ft.GetID().m_offset] = name;
  }

  uint32_t GetId(string const & name)
  {
    ASSERT(m_name2Id.find(name) != m_name2Id.end(), ());
    return m_name2Id[name];
  }

  string const & GetName(uint32_t id)
  {
    ASSERT(m_id2Name.find(id) != m_id2Name.end(), ());
    return m_id2Name[id];
  }
};

bool TestResult(IRoadGraph::TurnsVectorT const & vec, RoadPos const & pos, double d)
{
  return find_if(vec.begin(), vec.end(), EqualPos(pos, d)) != vec.end();
}

void FeatureID2Name(IRoadGraph::TurnsVectorT & vec, Name2IdMapping & mapping)
{
  for (size_t i = 0; i < vec.size(); ++i)
  {
    PossibleTurn & t = vec[i];
    string name = mapping.GetName(t.m_pos.GetFeatureId());
    int id = 0;
    bool isInt = strings::to_int(name, id);
    ASSERT(isInt, ());

    t.m_pos = RoadPos(static_cast<uint32_t>(id), t.m_pos.IsForward(), t.m_pos.GetPointId());
  }
}

}


UNIT_TEST(FRG_Smoke)
{
  classificator::Load();

  // ----- test 1 -----
  {
    Index index;
    m2::RectD rect;
    if (!index.Add("route_test1.mwm", rect))
    {
      LOG(LERROR, ("MWM file not found"));
      return;
    }

    FeatureRoadGraph graph(&index, 0);
    Name2IdMapping mapping;
    index.ForEachInRect(mapping, MercatorBounds::FullRect(), graph.GetStreetReadScale());

    {
      IRoadGraph::TurnsVectorT vec;
      graph.GetPossibleTurns(RoadPos(mapping.GetId("0"), true, 1), vec);
      FeatureID2Name(vec, mapping);
      TEST_EQUAL(vec.size(), 0, ());
    }

    {
      IRoadGraph::TurnsVectorT vec;
      graph.GetPossibleTurns(RoadPos(mapping.GetId("0"), false, 1), vec);
      FeatureID2Name(vec, mapping);
      TEST_EQUAL(vec.size(), 7, ());
      TEST(TestResult(vec, RoadPos(0, false, 2), 5), ());
      TEST(TestResult(vec, RoadPos(0, false, 3), 10), ());
      TEST(TestResult(vec, RoadPos(1, true, 1), 5), ());
      TEST(TestResult(vec, RoadPos(1, false, 2), 5), ());
      TEST(TestResult(vec, RoadPos(2, true, 0), 10), ());
      TEST(TestResult(vec, RoadPos(3, false, 0), 15), ());
      TEST(TestResult(vec, RoadPos(3, true, 2), 15), ());
    }

    {
      IRoadGraph::TurnsVectorT vec;
      graph.GetPossibleTurns(RoadPos(mapping.GetId("1"), true, 0), vec);
      FeatureID2Name(vec, mapping);
      TEST_EQUAL(vec.size(), 0, ());
    }

    {
      IRoadGraph::TurnsVectorT vec;
      graph.GetPossibleTurns(RoadPos(mapping.GetId("1"), false, 0), vec);
      FeatureID2Name(vec, mapping);
      TEST_EQUAL(vec.size(), 3, ());
      TEST(TestResult(vec, RoadPos(1, false, 2), 10), ());
      TEST(TestResult(vec, RoadPos(0, true, 1), 10), ());
      TEST(TestResult(vec, RoadPos(0, false, 2), 10), ());
    }

  }

  // ----- test 2 -----
  {
    Index index;
    m2::RectD rect;
    if (!index.Add("route_test2.mwm", rect))
    {
      LOG(LERROR, ("MWM file not found"));
      return;
    }

    FeatureRoadGraph graph(&index, 0);
    Name2IdMapping mapping;
    index.ForEachInRect(mapping, MercatorBounds::FullRect(), graph.GetStreetReadScale());

    {
      IRoadGraph::TurnsVectorT vec;
      graph.GetPossibleTurns(RoadPos(mapping.GetId("0"), false, 0), vec);
      FeatureID2Name(vec, mapping);
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
      graph.GetPossibleTurns(RoadPos(mapping.GetId("8"), true, 0), vec);
      FeatureID2Name(vec, mapping);
      TEST_EQUAL(vec.size(), 2, ());
      TEST(TestResult(vec, RoadPos(1, true, 1), -1), ());
      TEST(TestResult(vec, RoadPos(8, true, 5), -1), ());
    }

    {
      IRoadGraph::TurnsVectorT vec;
      graph.GetPossibleTurns(RoadPos(mapping.GetId("2"), true, 1), vec);
      FeatureID2Name(vec, mapping);
      TEST_EQUAL(vec.size(), 4, ());
      TEST(TestResult(vec, RoadPos(3, true, 0), -1), ());
      TEST(TestResult(vec, RoadPos(3, false, 1), -1), ());
      TEST(TestResult(vec, RoadPos(2, true, 0), -1), ());
      TEST(TestResult(vec, RoadPos(8, true, 4), -1), ());
    }

    {
      IRoadGraph::TurnsVectorT vec;
      graph.GetPossibleTurns(RoadPos(mapping.GetId("3"), false, 0), vec);
      FeatureID2Name(vec, mapping);
      TEST_EQUAL(vec.size(), 5, ());
      TEST(TestResult(vec, RoadPos(3, false, 1), -1), ());
      TEST(TestResult(vec, RoadPos(3, false, 2), -1), ());
      TEST(TestResult(vec, RoadPos(2, true, 0), -1), ());
      TEST(TestResult(vec, RoadPos(6, true, 0), -1), ());
      TEST(TestResult(vec, RoadPos(6, false, 1), -1), ());
    }

    {
      IRoadGraph::TurnsVectorT vec;
      graph.GetPossibleTurns(RoadPos(mapping.GetId("7"), false, 0), vec);
      FeatureID2Name(vec, mapping);
      TEST_EQUAL(vec.size(), 0, ());
    }

    {
      IRoadGraph::TurnsVectorT vec;
      graph.GetPossibleTurns(RoadPos(mapping.GetId("7"), true, 0), vec);
      FeatureID2Name(vec, mapping);
      TEST_EQUAL(vec.size(), 1, ());
      TEST(TestResult(vec, RoadPos(8, true, 1), -1), ());
    }

    {
      IRoadGraph::TurnsVectorT vec;
      graph.GetPossibleTurns(RoadPos(mapping.GetId("8"), true, 3), vec);
      FeatureID2Name(vec, mapping);
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
}
