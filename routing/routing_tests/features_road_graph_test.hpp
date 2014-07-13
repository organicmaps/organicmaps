#pragma once
#include "../features_road_graph.hpp"

#include "../../indexer/index.hpp"

#include "../../std/scoped_ptr.hpp"


namespace routing_test
{

class Name2IdMapping
{
  map<string, uint32_t> m_name2Id;
  map<uint32_t, string> m_id2Name;

public:
  void operator()(FeatureType const & ft);

  uint32_t GetId(string const & name);

  string const & GetName(uint32_t id);
};

class FeatureRoadGraphTester
{
  routing::FeaturesRoadGraph * m_graph;
  Name2IdMapping m_mapping;
  Index m_index;

  void FeatureID2Name(routing::RoadPos & pos);

public:
  FeatureRoadGraphTester(string const & name);

  routing::FeaturesRoadGraph * GetGraph() { return m_graph; }

  void FeatureID2Name(routing::IRoadGraph::TurnsVectorT & vec);
  void FeatureID2Name(vector<routing::RoadPos> & vec);
  void Name2FeatureID(vector<routing::RoadPos> & vec);

  void GetPossibleTurns(routing::RoadPos const & pos, routing::IRoadGraph::TurnsVectorT & vec, bool noOptimize = true);

  template <size_t N>
  void ReconstructPath(routing::RoadPos (&arr)[N], vector<m2::PointD> & vec);
};

}
