#include "routing/transit_world_graph.hpp"

#include "routing/index_graph.hpp"
#include "routing/transit_graph.hpp"

#include <utility>

namespace routing
{
using namespace std;

TransitWorldGraph::TransitWorldGraph(unique_ptr<CrossMwmGraph> crossMwmGraph,
                                     unique_ptr<IndexGraphLoader> indexLoader,
                                     unique_ptr<TransitGraphLoader> transitLoader,
                                     shared_ptr<EdgeEstimator> estimator)
  : m_crossMwmGraph(move(crossMwmGraph))
  , m_indexLoader(move(indexLoader))
  , m_transitLoader(move(transitLoader))
  , m_estimator(estimator)
{
  CHECK(m_indexLoader, ());
  CHECK(m_transitLoader, ());
  CHECK(m_estimator, ());
}

void TransitWorldGraph::GetEdgeList(Segment const & segment, bool isOutgoing, bool /* isLeap */,
                                    bool /* isEnding */, vector<SegmentEdge> & edges)
{
  auto & indexGraph = m_indexLoader->GetIndexGraph(segment.GetMwmId());
  // TransitGraph & transitGraph = m_transitLoader->GetTransitGraph(segment.GetMwmId());

  // TODO: Add fake connected to real segment.
  // TODO: Add real connected to fake segment.
  indexGraph.GetEdgeList(segment, isOutgoing, edges);

  if (m_mode != Mode::SingleMwm && m_crossMwmGraph && m_crossMwmGraph->IsTransition(segment, isOutgoing))
    GetTwins(segment, isOutgoing, edges);
}

Junction const & TransitWorldGraph::GetJunction(Segment const & segment, bool front)
{
  // TODO: fake transit segments.
  return GetRoadGeometry(segment.GetMwmId(), segment.GetFeatureId())
      .GetJunction(segment.GetPointId(front));
}

m2::PointD const & TransitWorldGraph::GetPoint(Segment const & segment, bool front)
{
  return GetJunction(segment, front).GetPoint();
}

RoadGeometry const & TransitWorldGraph::GetRoadGeometry(NumMwmId mwmId, uint32_t featureId)
{
  // TODO: fake transit segments.
  return m_indexLoader->GetIndexGraph(mwmId).GetGeometry().GetRoad(featureId);
}

void TransitWorldGraph::ClearCachedGraphs()
{
  m_indexLoader->Clear();
  m_transitLoader->Clear();
}

void TransitWorldGraph::GetOutgoingEdgesList(Segment const & segment, vector<SegmentEdge> & edges)
{
  edges.clear();
  GetEdgeList(segment, true /* isOutgoing */, false /* isLeap */, false /* isEnding */, edges);
}

void TransitWorldGraph::GetIngoingEdgesList(Segment const & segment, vector<SegmentEdge> & edges)
{
  edges.clear();
  GetEdgeList(segment, false /* isOutgoing */, false /* isLeap */, false /* isEnding */, edges);
}

RouteWeight TransitWorldGraph::HeuristicCostEstimate(Segment const & from, Segment const & to)
{
  // TODO: fake transit segments.
  return HeuristicCostEstimate(GetPoint(from, true /* front */), GetPoint(to, true /* front */));
}

RouteWeight TransitWorldGraph::HeuristicCostEstimate(m2::PointD const & from, m2::PointD const & to)
{
  return RouteWeight(m_estimator->CalcHeuristic(from, to), 0 /* nontransitCross */);
}

RouteWeight TransitWorldGraph::CalcSegmentWeight(Segment const & segment)
{
  // TODO: fake transit segments.
  return RouteWeight(m_estimator->CalcSegmentWeight(
                         segment, GetRoadGeometry(segment.GetMwmId(), segment.GetFeatureId())),
                     0 /* nontransitCross */);
}

RouteWeight TransitWorldGraph::CalcLeapWeight(m2::PointD const & from, m2::PointD const & to) const
{
  return RouteWeight(m_estimator->CalcLeapWeight(from, to), 0 /* nontransitCross */);
}

bool TransitWorldGraph::LeapIsAllowed(NumMwmId /* mwmId */) const { return false; }

void TransitWorldGraph::GetTwins(Segment const & segment, bool isOutgoing,
                                 vector<SegmentEdge> & edges)
{
  // TODO: GetTwins for fake transit segments.
  m_twins.clear();
  m_crossMwmGraph->GetTwins(segment, isOutgoing, m_twins);
  for (Segment const & twin : m_twins)
  {
    m2::PointD const & from = GetPoint(segment, true /* front */);
    m2::PointD const & to = GetPoint(twin, true /* front */);
    // Weight is usually zero because twins correspond the same feature
    // in different mwms. But if we have mwms with different versions and feature
    // was moved in one of them we can have nonzero weight here.
    double const weight = m_estimator->CalcHeuristic(from, to);
    edges.emplace_back(twin, RouteWeight(weight, 0 /* nontransitCross */));
  }
}
}  // namespace routing
