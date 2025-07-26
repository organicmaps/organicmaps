#pragma once
#include "routing_algorithm.hpp"

#include "routing/road_graph.hpp"

#include "routing_common/maxspeed_conversion.hpp"
#include "routing_common/vehicle_model.hpp"

#include <utility>
#include <vector>

namespace routing_test
{

class RoadGraphMockSource : public RoadGraphIFace
{
public:
  void AddRoad(RoadInfo && ri);

  inline size_t GetRoadCount() const { return m_roads.size(); }

  /// @name RoadGraphIFace overrides:
  /// @{
  RoadInfo GetRoadInfo(FeatureID const & f, routing::SpeedParams const & speedParams) const override;
  double GetSpeedKMpH(FeatureID const & featureId, routing::SpeedParams const & speedParams) const override;
  double GetMaxSpeedKMpH() const override;
  /// @}

  void ForEachFeatureClosestToCross(m2::PointD const & cross, ICrossEdgesLoader & edgeLoader) const override;
  void GetFeatureTypes(FeatureID const & featureId, feature::TypesHolder & types) const override;
  void GetJunctionTypes(geometry::PointWithAltitude const & junction, feature::TypesHolder & types) const override;
  routing::IRoadGraph::Mode GetMode() const override;

private:
  std::vector<RoadInfo> m_roads;
};

FeatureID MakeTestFeatureID(uint32_t offset);

void InitRoadGraphMockSourceWithTest1(RoadGraphMockSource & graphMock);
void InitRoadGraphMockSourceWithTest2(RoadGraphMockSource & graphMock);

}  // namespace routing_test
