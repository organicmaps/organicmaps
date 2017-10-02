#pragma once

#include "routing/num_mwm_id.hpp"
#include "routing/transit_graph.hpp"

#include "indexer/index.hpp"

#include <memory>
#include <unordered_map>

namespace routing
{
class TransitGraphLoader final
{
public:
  TransitGraphLoader(std::shared_ptr<NumMwmIds> numMwmIds, Index & index);

  TransitGraph & GetTransitGraph(NumMwmId mwmId);
  void Clear();

private:
  std::unique_ptr<TransitGraph> CreateTransitGraph(NumMwmId mwmId);

  Index & m_index;
  std::shared_ptr<NumMwmIds> m_numMwmIds;
  std::unordered_map<NumMwmId, std::unique_ptr<TransitGraph>> m_graphs;
};
}  // namespace routing
