#include "routing/world_graph.hpp"

namespace routing
{
using namespace std;

WorldGraph::WorldGraph(unique_ptr<CrossMwmGraph> crossMwmGraph, unique_ptr<IndexGraphLoader> loader,
                       shared_ptr<EdgeEstimator> estimator)
  : m_crossMwmGraph(move(crossMwmGraph)), m_loader(move(loader)), m_estimator(estimator)
{
  CHECK(m_loader, ());
  CHECK(m_estimator, ());
}

void WorldGraph::GetEdgeList(Segment const & segment, bool isOutgoing, bool isLeap,
                             std::vector<SegmentEdge> & edges)
{
  if (m_mode != Mode::NoLeaps && (isLeap || m_mode == Mode::LeapsOnly))
  {
    CHECK(m_crossMwmGraph, ());
    if (m_crossMwmGraph->IsTransition(segment, isOutgoing))
      GetTwins(segment, isOutgoing, edges);
    else
      m_crossMwmGraph->GetEdgeList(segment, isOutgoing, edges);
    return;
  }

  IndexGraph & indexGraph = GetIndexGraph(segment.GetMwmId());
  indexGraph.GetEdgeList(segment, isOutgoing, edges);

  if (m_crossMwmGraph && m_crossMwmGraph->IsTransition(segment, isOutgoing))
    GetTwins(segment, isOutgoing, edges);
}

Junction const & WorldGraph::GetJunction(Segment const & segment, bool front)
{
  return GetRoadGeometry(segment.GetMwmId(), segment.GetFeatureId())
      .GetJunction(segment.GetPointId(front));
}

m2::PointD const & WorldGraph::GetPoint(Segment const & segment, bool front)
{
  return GetJunction(segment, front).GetPoint();
}

RoadGeometry const & WorldGraph::GetRoadGeometry(NumMwmId mwmId, uint32_t featureId)
{
  return GetIndexGraph(mwmId).GetGeometry().GetRoad(featureId);
}

void WorldGraph::GetOutgoingEdgesList(Segment const & segment, vector<SegmentEdge> & edges)
{
  edges.clear();
  GetEdgeList(segment, true /* isOutgoing */, false /* isLeap */, edges);
}

void WorldGraph::GetIngoingEdgesList(Segment const & segment, vector<SegmentEdge> & edges)
{
  edges.clear();
  GetEdgeList(segment, false /* isOutgoing */, false /* isLeap */, edges);
}

RouteWeight WorldGraph::HeuristicCostEstimate(Segment const & from, Segment const & to)
{
  return RouteWeight(
      m_estimator->CalcHeuristic(GetPoint(from, true /* front */), GetPoint(to, true /* front */)),
      0);
}

void WorldGraph::GetTwins(Segment const & segment, bool isOutgoing, vector<SegmentEdge> & edges)
{
  m_twins.clear();
  m_crossMwmGraph->GetTwins(segment, isOutgoing, m_twins);
  for (Segment const & twin : m_twins)
  {
    m2::PointD const & from = GetPoint(segment, true /* front */);
    m2::PointD const & to = GetPoint(twin, true /* front */);
    double const weight = m_estimator->CalcHeuristic(from, to);
    edges.emplace_back(twin, RouteWeight(weight, 0));
  }
}

string DebugPrint(WorldGraph::Mode mode)
{
  switch (mode)
  {
  case WorldGraph::Mode::LeapsOnly: return "LeapsOnly";
  case WorldGraph::Mode::LeapsIfPossible: return "LeapsIfPossible";
  case WorldGraph::Mode::NoLeaps: return "NoLeaps";
  }
  ASSERT(false, ("Unknown mode:", static_cast<size_t>(mode)));
  return "Unknown mode";
}
}  // namespace routing
