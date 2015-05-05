#pragma once

#include "routing/road_graph.hpp"

namespace routing_test
{

class RoadGraphMockSource : public routing::IRoadGraph
{
public:
  void AddRoad(RoadInfo && ri);

  inline size_t GetRoadCount() const { return m_roads.size(); }

  // routing::IRoadGraph overrides:
  RoadInfo GetRoadInfo(uint32_t featureId) override;
  double GetSpeedKMPH(uint32_t featureId) override;
  double GetMaxSpeedKMPH() override;
  void ForEachFeatureClosestToCross(m2::PointD const & cross,
                                    CrossEdgesLoader & edgeLoader) override;

private:
  vector<RoadInfo> m_roads;
};

void InitRoadGraphMockSourceWithTest1(RoadGraphMockSource & graphMock);
void InitRoadGraphMockSourceWithTest2(RoadGraphMockSource & graphMock);

}  // namespace routing_test
