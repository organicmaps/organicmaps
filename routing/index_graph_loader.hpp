#pragma once

#include "routing/edge_estimator.hpp"
#include "routing/index_graph.hpp"
#include "routing/vehicle_mask.hpp"

#include "routing_common/num_mwm_id.hpp"
#include "routing_common/vehicle_model.hpp"

#include "indexer/index.hpp"

#include <memory>

namespace routing
{
class IndexGraphLoader
{
public:
  virtual ~IndexGraphLoader() = default;

  virtual IndexGraph & GetIndexGraph(NumMwmId mwmId) = 0;
  virtual void Clear() = 0;

  static std::unique_ptr<IndexGraphLoader> Create(
      VehicleType vehicleType, bool loadAltitudes, std::shared_ptr<NumMwmIds> numMwmIds,
      std::shared_ptr<VehicleModelFactoryInterface> vehicleModelFactory,
      std::shared_ptr<EdgeEstimator> estimator, Index & index);
};

void DeserializeIndexGraph(MwmValue const & mwmValue, VehicleType vehicleType, IndexGraph & graph);
}  // namespace routing
