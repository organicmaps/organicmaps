#include "routing/leaps_graph.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "base/assert.hpp"

#include <set>

namespace routing
{
LeapsGraph::LeapsGraph(IndexGraphStarter & starter) : m_starter(starter)
{
  m_startPoint = m_starter.GetPoint(m_starter.GetStartSegment(), true /* front */);
  m_finishPoint = m_starter.GetPoint(m_starter.GetFinishSegment(), true /* front */);
  m_startSegment = m_starter.GetStartSegment();
  m_finishSegment = m_starter.GetFinishSegment();
}

void LeapsGraph::GetOutgoingEdgesList(Segment const & segment,
                                      std::vector<SegmentEdge> & edges)
{
  GetEdgesList(segment, true /* isOutgoing */, edges);
}

void LeapsGraph::GetIngoingEdgesList(Segment const & segment,
                                     std::vector<SegmentEdge> & edges)
{
  GetEdgesList(segment, false /* isOutgoing */, edges);
}

RouteWeight LeapsGraph::HeuristicCostEstimate(Segment const & from, Segment const & to)
{
  ASSERT(to == m_startSegment || to == m_finishSegment, ());
  bool const toFinish = to == m_finishSegment;
  auto const & toPoint = toFinish ? m_finishPoint : m_startPoint;
  return m_starter.HeuristicCostEstimate(from, toPoint);
}

void LeapsGraph::GetEdgesList(Segment const & segment, bool isOutgoing,
                              std::vector<SegmentEdge> & edges)
{
  edges.clear();

  if (segment == m_startSegment)
    return GetEdgesListFromStart(segment, isOutgoing, edges);

  if (segment == m_finishSegment)
    return GetEdgesListToFinish(segment, isOutgoing, edges);

  if (!m_starter.IsRoutingOptionsGood(segment))
    return;

  auto & crossMwmGraph = m_starter.GetGraph().GetCrossMwmGraph();
  if (crossMwmGraph.IsTransition(segment, isOutgoing))
  {
    std::vector<Segment> twins;
    m_starter.GetGraph().GetTwinsInner(segment, isOutgoing, twins);
    for (auto const & twin : twins)
      edges.emplace_back(twin, RouteWeight(0.0));

    return;
  }

  if (isOutgoing)
    crossMwmGraph.GetOutgoingEdgeList(segment, edges);
  else
    crossMwmGraph.GetIngoingEdgeList(segment, edges);
}

void LeapsGraph::GetEdgesListFromStart(Segment const & segment, bool isOutgoing,
                                       std::vector<SegmentEdge> & edges)
{
  CHECK(isOutgoing, ());
  for (auto const mwmId : m_starter.GetStartEnding().m_mwmIds)
  {
    // Connect start to all exits (|isEnter| == false).
    auto const & exits = m_starter.GetGraph().GetTransitions(mwmId, false /* isEnter */);
    for (uint32_t exitId = 0; exitId < static_cast<uint32_t>(exits.size()); ++exitId)
    {
      auto const & exit = exits[exitId];
      auto const & exitFrontPoint = m_starter.GetPoint(exit, true /* front */);
      auto const weight = m_starter.GetGraph().CalcLeapWeight(m_startPoint, exitFrontPoint);

      edges.emplace_back(exit, weight);
    }
  }
}

void LeapsGraph::GetEdgesListToFinish(Segment const & segment, bool isOutgoing,
                                      std::vector<SegmentEdge> & edges)
{
  CHECK(!isOutgoing, ());
  for (auto const mwmId : m_starter.GetFinishEnding().m_mwmIds)
  {
    // Connect finish to all enters (|isEnter| == true).
    auto const & enters = m_starter.GetGraph().GetTransitions(mwmId, true /* isEnter */);
    for (uint32_t enterId = 0; enterId < static_cast<uint32_t>(enters.size()); ++enterId)
    {
      auto const & enter = enters[enterId];
      auto const & enterFrontPoint = m_starter.GetPoint(enter, true /* front */);
      auto const weight = m_starter.GetGraph().CalcLeapWeight(enterFrontPoint, m_finishPoint);

      edges.emplace_back(enter, weight);
    }
  }
}

m2::PointD const & LeapsGraph::GetPoint(Segment const & segment, bool front)
{
  return m_starter.GetPoint(segment, front);
}

Segment const & LeapsGraph::GetStartSegment() const
{
  return m_startSegment;
}

Segment const & LeapsGraph::GetFinishSegment() const
{
  return m_finishSegment;
}

RouteWeight LeapsGraph::GetAStarWeightEpsilon()
{
  return m_starter.GetAStarWeightEpsilon();
}
}  // namespace routing
