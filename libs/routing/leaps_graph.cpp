#include "routing/leaps_graph.hpp"

#include "routing/cross_mwm_graph.hpp"
#include "routing/index_graph_starter.hpp"

#include "base/assert.hpp"

#include <set>
#include <utility>

namespace routing
{
LeapsGraph::LeapsGraph(IndexGraphStarter & starter, MwmHierarchyHandler && hierarchyHandler)
  : m_starter(starter)
  , m_hierarchyHandler(std::move(hierarchyHandler))
{
  m_startPoint = m_starter.GetPoint(m_starter.GetStartSegment(), true /* front */);
  m_finishPoint = m_starter.GetPoint(m_starter.GetFinishSegment(), true /* front */);
  m_startSegment = m_starter.GetStartSegment();
  m_finishSegment = m_starter.GetFinishSegment();
}

void LeapsGraph::GetOutgoingEdgesList(astar::VertexData<Vertex, Weight> const & vertexData, EdgeListT & edges)
{
  GetEdgesList(vertexData.m_vertex, true /* isOutgoing */, edges);
}

void LeapsGraph::GetIngoingEdgesList(astar::VertexData<Vertex, Weight> const & vertexData, EdgeListT & edges)
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

void LeapsGraph::GetEdgesList(Segment const & segment, bool isOutgoing, EdgeListT & edges)
{
  edges.clear();

  if (segment == m_startSegment)
  {
    CHECK(isOutgoing, ("Only forward wave of A* should get edges from start. Backward wave should "
                       "stop when first time visit the |m_startSegment|."));
    return GetEdgesListFromStart(edges);
  }

  if (segment == m_finishSegment)
  {
    CHECK(!isOutgoing, ("Only backward wave of A* should get edges to finish. Forward wave should "
                        "stop when first time visit the |m_finishSegment|."));
    return GetEdgesListToFinish(edges);
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

void LeapsGraph::GetEdgesListFromStart(EdgeListT & edges) const
{
  for (auto const mwmId : m_starter.GetStartMwms())
  {
    // Connect start to all exits (|isEnter| == false).
    m_starter.GetGraph().ForEachTransition(mwmId, false /* isEnter */, [&](Segment const & exit)
    {
      auto const & exitFrontPoint = m_starter.GetPoint(exit, true /* front */);
      auto const weight = m_starter.GetGraph().CalcLeapWeight(m_startPoint, exitFrontPoint, mwmId);

      edges.emplace_back(exit, weight);
    });
  }
}

void LeapsGraph::GetEdgesListToFinish(EdgeListT & edges) const
{
  for (auto const mwmId : m_starter.GetFinishMwms())
  {
    // Connect finish to all enters (|isEnter| == true).
    m_starter.GetGraph().ForEachTransition(mwmId, true /* isEnter */, [&](Segment const & enter)
    {
      auto const & enterFrontPoint = m_starter.GetPoint(enter, true /* front */);
      auto const weight = m_starter.GetGraph().CalcLeapWeight(enterFrontPoint, m_finishPoint, mwmId);

      edges.emplace_back(enter, weight);
    });
  }
}

ms::LatLon const & LeapsGraph::GetPoint(Segment const & segment, bool front) const
{
  return m_starter.GetPoint(segment, front);
}

RouteWeight LeapsGraph::GetAStarWeightEpsilon()
{
  return m_starter.GetAStarWeightEpsilon();
}

RouteWeight LeapsGraph::CalcMiddleCrossMwmWeight(std::vector<Segment> const & path)
{
  ASSERT_GREATER(path.size(), 1, ());
  auto & crossMwmGraph = m_starter.GetGraph().GetCrossMwmGraph();

  RouteWeight res;
  for (size_t i = 1; i < path.size() - 2; ++i)
  {
    auto const & from = path[i];
    auto const & to = path[i + 1];
    NumMwmId const fromMwm = from.GetMwmId();
    NumMwmId const toMwm = to.GetMwmId();
    ASSERT(fromMwm != kFakeNumMwmId && toMwm != kFakeNumMwmId, ());

    if (fromMwm != toMwm)
      res += m_hierarchyHandler.GetCrossBorderPenalty(fromMwm, toMwm);
    else
      res += crossMwmGraph.GetWeightSure(from, to);
  }

  return res;
}

}  // namespace routing
