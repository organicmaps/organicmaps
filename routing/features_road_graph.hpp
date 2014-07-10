#pragma once

#include "road_graph.hpp"


class Index;

namespace feature
{
  class TypesHolder;
}

namespace routing
{

class FeatureRoadGraph : public IRoadGraph
{
public:
  FeatureRoadGraph(Index * pIndex, size_t mwmID);

  virtual void GetPossibleTurns(RoadPos const & pos, vector<PossibleTurn> & turns);
  virtual double GetFeatureDistance(RoadPos const & p1, RoadPos const & p2);
  virtual void ReconstructPath(RoadPosVectorT const & positions, PointsVectorT & poly);

  static uint32_t GetStreetReadScale();

private:
  friend class CrossFeaturesLoader;

  bool IsStreet(feature::TypesHolder const & types) const;
  bool IsOneway(feature::TypesHolder const & types) const;

private:
  uint32_t m_onewayType;

  Index * m_pIndex;
  size_t m_mwmID;
};

} // namespace routing
