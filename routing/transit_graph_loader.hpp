#pragma once

#include "routing/edge_estimator.hpp"
#include "routing/index_graph.hpp"
#include "routing/transit_graph.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "indexer/index.hpp"

#include <memory>
#include <unordered_map>

namespace routing
{
class TransitGraphLoader final
{
public:
  TransitGraphLoader(std::shared_ptr<NumMwmIds> numMwmIds, Index & index,
                     std::shared_ptr<EdgeEstimator> estimator);

  TransitGraph & GetTransitGraph(NumMwmId mwmId, IndexGraph & indexGraph);
  void Clear();

private:
  std::unique_ptr<TransitGraph> CreateTransitGraph(NumMwmId mwmId, IndexGraph & indexGraph) const;

  Index & m_index;
  std::shared_ptr<NumMwmIds> m_numMwmIds;
  std::shared_ptr<EdgeEstimator> m_estimator;
  std::unordered_map<NumMwmId, std::unique_ptr<TransitGraph>> m_graphs;
};
}  // namespace routing
