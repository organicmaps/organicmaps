#pragma once

#include "routing/cross_mwm_index_graph.hpp"
#include "routing/edge_estimator.hpp"
#include "routing/geometry.hpp"
#include "routing/index_graph.hpp"
#include "routing/index_graph_loader.hpp"
#include "routing/segment.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace routing
{
class WorldGraph final
{
public:
  WorldGraph(std::unique_ptr<CrossMwmIndexGraph> crossMwmGraph,
             std::unique_ptr<IndexGraphLoader> loader, std::shared_ptr<EdgeEstimator> estimator);

  void GetEdgeList(Segment const & segment, bool isOutgoing, std::vector<SegmentEdge> & edges);

  IndexGraph & GetIndexGraph(NumMwmId numMwmId) { return m_loader->GetIndexGraph(numMwmId); }
  EdgeEstimator const & GetEstimator() const { return *m_estimator; }

  RoadGeometry const & GetRoadGeometry(NumMwmId mwmId, uint32_t featureId);

private:
  std::unique_ptr<CrossMwmIndexGraph> m_crossMwmGraph;
  std::unique_ptr<IndexGraphLoader> m_loader;
  std::shared_ptr<EdgeEstimator> m_estimator;
  std::vector<Segment> m_twins;
};
}  // namespace routing
