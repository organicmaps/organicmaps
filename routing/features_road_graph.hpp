#pragma once

#include "road_graph.hpp"

class Index;
class FeatureType;

namespace feature
{
  class TypesHolder;
}

namespace routing
{

class FeatureRoadGraph : public IRoadGraph
{
public:
  FeatureRoadGraph(Index const * pIndex, size_t mwmID);

  virtual void GetPossibleTurns(RoadPos const & pos, vector<PossibleTurn> & turns, bool noOptimize = true);
  virtual void ReconstructPath(RoadPosVectorT const & positions, Route & route);

  static uint32_t GetStreetReadScale();

  inline size_t GetMwmID() const { return m_mwmID; }

private:
  friend class CrossFeaturesLoader;

  bool IsStreet(feature::TypesHolder const & types) const;
  bool IsOneway(feature::TypesHolder const & types) const;

  void LoadFeature(uint32_t id, FeatureType & ft);

private:
  Index const * m_pIndex;
  size_t m_mwmID;
};

} // namespace routing
