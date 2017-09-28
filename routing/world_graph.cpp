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
                             bool isEnding, std::vector<SegmentEdge> & edges)
{
  if (IsInSingleMwmMode())
  {
    m_loader->GetIndexGraph(m_mwmId).GetEdgeList(segment, isOutgoing, edges);
    return;
  }

  // If mode is LeapsOnly and |isEnding| == true we need to connect segment to transitions.
  // If |isOutgoing| == true connects |segment| with all exits of mwm.
  // If |isOutgoing| == false connects all enters to mwm with |segment|.
  if (m_mode == Mode::LeapsOnly && isEnding)
  {
    edges.clear();
    m2::PointD const & segmentPoint = GetPoint(segment, true /* front */);
  
    // Note. If |isOutgoing| == true it's necessary to add edges which connect the start with all
    // exits of its mwm. So |isEnter| below should be set to false.
    // If |isOutgoing| == false all enters of the finish mwm should be connected with the finish
    // point. So |isEnter| below should be set to true.
    m_crossMwmGraph->ForEachTransition(
                            segment.GetMwmId(), !isOutgoing /* isEnter */, [&](Segment const & transition) {
                              edges.emplace_back(transition, CalcLeapWeight(segmentPoint,
                                                                            GetPoint(transition, isOutgoing)));
                            });
    return;
  }

  if (m_mode != Mode::NoLeaps && (isLeap || m_mode == Mode::LeapsOnly))
  {
    CHECK(m_crossMwmGraph, ());
    if (m_crossMwmGraph->IsTransition(segment, isOutgoing))
      GetTwins(segment, isOutgoing, edges);
    else
      m_crossMwmGraph->GetEdgeList(segment, isOutgoing, edges);
    return;
  }

  IndexGraph & indexGraph = m_loader->GetIndexGraph(segment.GetMwmId());
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
  return m_loader->GetIndexGraph(mwmId).GetGeometry().GetRoad(featureId);
}

void WorldGraph::GetOutgoingEdgesList(Segment const & segment, vector<SegmentEdge> & edges)
{
  edges.clear();
  GetEdgeList(segment, true /* isOutgoing */, false /* isLeap */, false /* isEnding */, edges);
}

void WorldGraph::GetIngoingEdgesList(Segment const & segment, vector<SegmentEdge> & edges)
{
  edges.clear();
  GetEdgeList(segment, false /* isOutgoing */, false /* isLeap */, false /* isEnding */, edges);
}

RouteWeight WorldGraph::HeuristicCostEstimate(Segment const & from, Segment const & to)
{
  return HeuristicCostEstimate(GetPoint(from, true /* front */),
                               GetPoint(to, true /* front */));
}

RouteWeight WorldGraph::HeuristicCostEstimate(m2::PointD const & from, m2::PointD const & to)
{
  return RouteWeight(m_estimator->CalcHeuristic(from, to), 0 /* nontransitCross */);
}


RouteWeight WorldGraph::CalcSegmentWeight(Segment const & segment)
{
  return RouteWeight(
      m_estimator->CalcSegmentWeight(segment, GetRoadGeometry(segment.GetMwmId(), segment.GetFeatureId())),
      0 /* nontransitCross */);
}

RouteWeight WorldGraph::CalcLeapWeight(m2::PointD const & from, m2::PointD const & to) const
{
  return RouteWeight(m_estimator->CalcLeapWeight(from, to), 0 /* nontransitCross */);
}

bool WorldGraph::LeapIsAllowed(NumMwmId mwmId) const
{
  return m_estimator->LeapIsAllowed(mwmId);
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
    edges.emplace_back(twin, RouteWeight(weight, 0 /* nontransitCross */));
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
