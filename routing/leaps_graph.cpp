#include "routing/leaps_graph.hpp"

#include "routing_common/num_mwm_id.hpp"

#include <set>

namespace routing
{
LeapsGraph::LeapsGraph(IndexGraphStarter & starter) : m_starter(starter) {}

void LeapsGraph::GetOutgoingEdgesList(Segment const & segment, std::vector<SegmentEdge> & edges)
{
  GetEdgesList(segment, true /* isOutgoing */, edges);
}

void LeapsGraph::GetIngoingEdgesList(Segment const & segment, std::vector<SegmentEdge> & edges)
{
  GetEdgesList(segment, false /* isOutgoing */, edges);
}

RouteWeight LeapsGraph::HeuristicCostEstimate(Segment const & from, Segment const & to)
{
  return m_starter.HeuristicCostEstimate(from, to);
}

void LeapsGraph::GetEdgesList(Segment const & segment, bool isOutgoing,
                              std::vector<SegmentEdge> & edges)
{
  // Ingoing edges listing are not supported in LeapsOnly mode because we do not have enough
  // information to calculate |segment| weight. See https://jira.mail.ru/browse/MAPSME-5743 for
  // details.
  CHECK(isOutgoing, ("Ingoing edges listing are not supported in LeapsOnly mode."));

  edges.clear();

  if (segment.IsRealSegment() && !m_starter.IsRoutingOptionsGood(segment))
    return;

  if (segment == m_starter.GetStartSegment())
  {
    GetEdgesListForStart(segment, isOutgoing, edges);
    return;
  }

  // An edge from finish mwm enter to finish.
  if (m_starter.GetFinishEnding().OverlapsWithMwm(segment.GetMwmId()))
  {
    GetEdgesListForFinish(segment, isOutgoing, edges);
    return;
  }

  auto & crossMwmGraph = m_starter.GetGraph().GetCrossMwmGraph();
  if (crossMwmGraph.IsTransition(segment, isOutgoing))
  {
    std::vector<Segment> twins;
    m_starter.GetGraph().GetTwinsInner(segment, isOutgoing, twins);
    for (auto const & twin : twins)
      edges.emplace_back(twin, RouteWeight(0.0));
  }
  else
  {
    crossMwmGraph.GetOutgoingEdgeList(segment, edges);
  }
}

void LeapsGraph::GetEdgesListForStart(Segment const & segment, bool isOutgoing,
                                      std::vector<SegmentEdge> & edges)
{
  auto const & segmentPoint = GetPoint(segment, true /* front */);
  std::set<NumMwmId> seen;
  for (auto const mwmId : m_starter.GetStartEnding().m_mwmIds)
  {
    if (seen.insert(mwmId).second)
    {
      // Connect start to all exits (|isEnter| == false).
      for (auto const & transition : m_starter.GetGraph().GetTransitions(mwmId, false /* isEnter */))
      {
        auto const & transitionFrontPoint = GetPoint(transition, true /* front */);
        auto const weight = m_starter.GetGraph().CalcLeapWeight(segmentPoint, transitionFrontPoint);
        edges.emplace_back(transition, weight);
      }
    }
  }
}

void LeapsGraph::GetEdgesListForFinish(Segment const & segment, bool isOutgoing,
                                       std::vector<SegmentEdge> & edges)
{
  auto const & segmentPoint = GetPoint(segment, true /* front */);
  edges.emplace_back(m_starter.GetFinishSegment(),
                     m_starter.GetGraph().CalcLeapWeight(
                         segmentPoint, GetPoint(m_starter.GetFinishSegment(), true /* front */)));
}

m2::PointD const & LeapsGraph::GetPoint(Segment const & segment, bool front) const
{
  return m_starter.GetPoint(segment, front);
}

RouteWeight LeapsGraph::GetAStarWeightEpsilon()
{
  return m_starter.GetAStarWeightEpsilon();
}
}  // namespace routing
