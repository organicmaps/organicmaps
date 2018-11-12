#pragma once

#include "routing/road_graph.hpp"

#include "routing_common/maxspeed_conversion.hpp"
#include "routing_common/vehicle_model.hpp"

namespace routing_test
{

class RoadGraphMockSource : public routing::IRoadGraph
{
public:
  void AddRoad(RoadInfo && ri);

  inline size_t GetRoadCount() const { return m_roads.size(); }

  // routing::IRoadGraph overrides:
  RoadInfo GetRoadInfo(FeatureID const & f, routing::SpeedParams const & speedParams) const override;
  double GetSpeedKMpH(FeatureID const & featureId,
                      routing::SpeedParams const & speedParams) const override;
  double GetMaxSpeedKMpH() const override;
  void ForEachFeatureClosestToCross(m2::PointD const & cross,
                                    ICrossEdgesLoader & edgeLoader) const override;
  void FindClosestEdges(m2::PointD const & point, uint32_t count,
                        vector<pair<routing::Edge, routing::Junction>> & vicinities) const override;
  void GetFeatureTypes(FeatureID const & featureId, feature::TypesHolder & types) const override;
  void GetJunctionTypes(routing::Junction const & junction, feature::TypesHolder & types) const override;
  routing::IRoadGraph::Mode GetMode() const override;

private:
  vector<RoadInfo> m_roads;
};

FeatureID MakeTestFeatureID(uint32_t offset);

void InitRoadGraphMockSourceWithTest1(RoadGraphMockSource & graphMock);
void InitRoadGraphMockSourceWithTest2(RoadGraphMockSource & graphMock);

}  // namespace routing_test
