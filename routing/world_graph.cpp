#include "routing/world_graph.hpp"

namespace routing
{
using namespace std;

WorldGraph::WorldGraph(unique_ptr<CrossMwmIndexGraph> crossMwmGraph,
                       unique_ptr<IndexGraphLoader> loader, shared_ptr<EdgeEstimator> estimator)
  : m_crossMwmGraph(move(crossMwmGraph)), m_loader(move(loader)), m_estimator(estimator)
{
  CHECK(m_loader, ());
  CHECK(m_estimator, ());
}

void WorldGraph::GetEdgeList(Segment const & segment, bool isOutgoing, vector<SegmentEdge> & edges)
{
  IndexGraph & indexGraph = GetIndexGraph(segment.GetMwmId());
  indexGraph.GetEdgeList(segment, isOutgoing, edges);

  // TODO remove this return to activate m_crossMwmGraph.
  return;

  if (!m_crossMwmGraph || !m_crossMwmGraph->IsTransition(segment, isOutgoing))
    return;

  m_twins.clear();
  m_crossMwmGraph->GetTwins(segment, isOutgoing, m_twins);
  for (Segment const & twin : m_twins)
    edges.emplace_back(twin, 0.0 /* weight */);
}

RoadGeometry const & WorldGraph::GetRoadGeometry(NumMwmId mwmId, uint32_t featureId)
{
  return GetIndexGraph(mwmId).GetGeometry().GetRoad(featureId);
}
}  // namespace routing
