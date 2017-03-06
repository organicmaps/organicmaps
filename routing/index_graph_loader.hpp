#pragma once

#include "routing/edge_estimator.hpp"
#include "routing/index_graph.hpp"
#include "routing/num_mwm_id.hpp"

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
      std::shared_ptr<NumMwmIds> numMwmIds,
      std::shared_ptr<VehicleModelFactory> vehicleModelFactory,
      std::shared_ptr<EdgeEstimator> estimator, Index & index);
};
}  // namespace routing
