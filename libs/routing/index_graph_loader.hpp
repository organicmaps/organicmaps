#pragma once

#include "routing/edge_estimator.hpp"
#include "routing/index_graph.hpp"
#include "routing/route.hpp"
#include "routing/speed_camera_ser_des.hpp"
#include "routing/vehicle_mask.hpp"

#include "routing_common/num_mwm_id.hpp"
#include "routing_common/vehicle_model.hpp"

#include <memory>
#include <vector>

class MwmValue;

namespace routing
{
class MwmDataSource;

class IndexGraphLoader
{
public:
  virtual ~IndexGraphLoader() = default;

  virtual IndexGraph & GetIndexGraph(NumMwmId mwmId) = 0;
  virtual Geometry & GetGeometry(NumMwmId numMwmId) = 0;

  // Because several cameras can lie on one segment we return vector of them.
  virtual std::vector<RouteSegment::SpeedCamera> GetSpeedCameraInfo(Segment const & segment) = 0;
  virtual void Clear() = 0;

  static std::unique_ptr<IndexGraphLoader> Create(VehicleType vehicleType, bool loadAltitudes,
                                                  std::shared_ptr<VehicleModelFactoryInterface> vehicleModelFactory,
                                                  std::shared_ptr<EdgeEstimator> estimator, MwmDataSource & dataSource,
                                                  RoutingOptions routingOptions = RoutingOptions());
};

void DeserializeIndexGraph(MwmValue const & mwmValue, VehicleType vehicleType, IndexGraph & graph);

uint32_t DeserializeIndexGraphNumRoads(MwmValue const & mwmValue, VehicleType vehicleType);

bool ReadRoadAccessFromMwm(MwmValue const & mwmValue, VehicleType vehicleType, RoadAccess & roadAccess);
bool ReadSpeedCamsFromMwm(MwmValue const & mwmValue, SpeedCamerasMapT & camerasMap);
}  // namespace routing
