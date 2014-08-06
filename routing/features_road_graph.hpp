#pragma once

#include "road_graph.hpp"
#include "../std/unique_ptr.hpp"

class Index;
class FeatureType;

namespace feature
{
  class TypesHolder;
}

namespace routing
{

class IVehicleModel;

class FeaturesRoadGraph : public IRoadGraph
{
public:
  FeaturesRoadGraph(Index const * pIndex, size_t mwmID);

  virtual void GetPossibleTurns(RoadPos const & pos, vector<PossibleTurn> & turns, bool noOptimize = true);
  virtual void ReconstructPath(RoadPosVectorT const & positions, Route & route);

  static uint32_t GetStreetReadScale();

  inline size_t GetMwmID() const { return m_mwmID; }

private:
  friend class CrossFeaturesLoader;

  bool IsOneWay(FeatureType const & ft) const;
  double GetSpeed(FeatureType const & ft) const;
  void LoadFeature(uint32_t id, FeatureType & ft);

private:
  Index const * m_pIndex;
  size_t m_mwmID;
  unique_ptr<IVehicleModel> const m_vehicleModel;
};

} // namespace routing
