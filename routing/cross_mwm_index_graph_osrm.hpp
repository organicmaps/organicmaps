#pragma once

#include "routing/cross_mwm_index_graph.hpp"
#include "routing/num_mwm_id.hpp"
#include "routing/routing_mapping.hpp"

#include "base/math.hpp"

#include <map>
#include <memory>
#include <set>

namespace routing
{
class CrossMwmIndexGraphOsrm : public CrossMwmIndexGraph
{
public:
  CrossMwmIndexGraphOsrm(std::shared_ptr<NumMwmIds> numMwmIds, RoutingIndexManager & indexManager)
    : m_indexManager(indexManager), m_numMwmIds(numMwmIds)
  {
  }

  // CrossMwmIndexGraph overrides:
  bool IsTransition(Segment const & s, bool isOutgoing) override;
  void GetTwin(Segment const & s, std::vector<Segment> & twins) const override;
  void GetEdgeList(Segment const & s, bool isOutgoing, std::vector<SegmentEdge> & edges) const override;

private:
  struct TransitionSegments
  {
    std::set<Segment> m_ingoing;
    std::set<Segment> m_outgoing;
  };

  RoutingIndexManager & m_indexManager;
  std::shared_ptr<NumMwmIds> m_numMwmIds;

  std::map<NumMwmId, TransitionSegments> m_transitionCache;
};
}  // namespace routing
