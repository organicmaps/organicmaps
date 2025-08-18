#pragma once

#include "routing/edge_estimator.hpp"
#include "routing/index_graph.hpp"
#include "routing/transit_graph.hpp"

#include "routing_common/num_mwm_id.hpp"

#include <memory>

namespace routing
{
class MwmDataSource;

class TransitGraphLoader
{
public:
  virtual ~TransitGraphLoader() = default;

  virtual TransitGraph & GetTransitGraph(NumMwmId mwmId, IndexGraph & indexGraph) = 0;
  virtual void Clear() = 0;

  static std::unique_ptr<TransitGraphLoader> Create(MwmDataSource & dataSource,
                                                    std::shared_ptr<EdgeEstimator> estimator);
};
}  // namespace routing
