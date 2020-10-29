#include "routing/leaps_graph.hpp"

#include "base/assert.hpp"

#include <set>
#include <utility>

namespace routing
{
LeapsGraph::LeapsGraph(IndexGraphStarter & starter, MwmHierarchyHandler && hierarchyHandler)
  : m_starter(starter), m_hierarchyHandler(std::move(hierarchyHandler))
{
  m_startPoint = m_starter.GetPoint(m_starter.GetStartSegment(), true /* front */);
  m_finishPoint = m_starter.GetPoint(m_starter.GetFinishSegment(), true /* front */);
  m_startSegment = m_starter.GetStartSegment();
  m_finishSegment = m_starter.GetFinishSegment();
}

void LeapsGraph::GetOutgoingEdgesList(astar::VertexData<Vertex, Weight> const & vertexData,
                                      std::vector<SegmentEdge> & edges)
{
  GetEdgesList(vertexData.m_vertex, true /* isOutgoing */, edges);
}

void LeapsGraph::GetIngoingEdgesList(astar::VertexData<Vertex, Weight> const & vertexData,
                                     std::vector<SegmentEdge> & edges)
{
  GetEdgesList(vertexData.m_vertex, false /* isOutgoing */, edges);
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
  {
    CHECK(isOutgoing, ("Only forward wave of A* should get edges from start. Backward wave should "
                       "stop when first time visit the |m_startSegment|."));
    return GetEdgesListFromStart(segment, edges);
  }

  if (segment == m_finishSegment)
  {
    CHECK(!isOutgoing, ("Only backward wave of A* should get edges to finish. Forward wave should "
                        "stop when first time visit the |m_finishSegment|."));
    return GetEdgesListToFinish(segment, edges);
  }

  if (!m_starter.IsRoutingOptionsGood(segment))
    return;

  auto & crossMwmGraph = m_starter.GetGraph().GetCrossMwmGraph();

  if (crossMwmGraph.IsTransition(segment, isOutgoing))
  {
    auto const segMwmId = segment.GetMwmId();

    std::vector<Segment> twins;
    m_starter.GetGraph().GetTwinsInner(segment, isOutgoing, twins);
    for (auto const & twin : twins)
      edges.emplace_back(twin, m_hierarchyHandler.GetCrossBorderPenalty(segMwmId, twin.GetMwmId()));

    return;
  }

  if (isOutgoing)
    crossMwmGraph.GetOutgoingEdgeList(segment, edges);
  else
    crossMwmGraph.GetIngoingEdgeList(segment, edges);
}

void LeapsGraph::GetEdgesListFromStart(Segment const & segment, std::vector<SegmentEdge> & edges)
{
  for (auto const mwmId : m_starter.GetStartEnding().m_mwmIds)
  {
    // Connect start to all exits (|isEnter| == false).
    auto const & exits = m_starter.GetGraph().GetTransitions(mwmId, false /* isEnter */);
    for (auto const & exit : exits)
    {
      auto const & exitFrontPoint = m_starter.GetPoint(exit, true /* front */);
      auto const weight = m_starter.GetGraph().CalcLeapWeight(m_startPoint, exitFrontPoint, mwmId);

      edges.emplace_back(exit, weight);
    }
  }
}

void LeapsGraph::GetEdgesListToFinish(Segment const & segment, std::vector<SegmentEdge> & edges)
{
  for (auto const mwmId : m_starter.GetFinishEnding().m_mwmIds)
  {
    // Connect finish to all enters (|isEnter| == true).
    auto const & enters = m_starter.GetGraph().GetTransitions(mwmId, true /* isEnter */);
    for (auto const & enter : enters)
    {
      auto const & enterFrontPoint = m_starter.GetPoint(enter, true /* front */);
      auto const weight = m_starter.GetGraph().CalcLeapWeight(enterFrontPoint, m_finishPoint, mwmId);

      edges.emplace_back(enter, weight);
    }
  }
}

ms::LatLon const & LeapsGraph::GetPoint(Segment const & segment, bool front) const
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
