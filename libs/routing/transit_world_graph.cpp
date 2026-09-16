#include "routing/transit_world_graph.hpp"

#include "routing/index_graph.hpp"
#include "routing/transit_graph.hpp"

#include "geometry/distance_on_sphere.hpp"

#include <memory>
#include <utility>

namespace routing
{
TransitWorldGraph::TransitWorldGraph(std::unique_ptr<CrossMwmGraph> crossMwmGraph,
                                     std::unique_ptr<IndexGraphLoader> indexLoader,
                                     std::unique_ptr<TransitGraphLoader> transitLoader,
                                     std::shared_ptr<EdgeEstimator> estimator)
  : m_crossMwmGraph(std::move(crossMwmGraph))
  , m_indexLoader(std::move(indexLoader))
  , m_transitLoader(std::move(transitLoader))
  , m_estimator(std::move(estimator))
{
  CHECK(m_indexLoader, ());
  CHECK(m_transitLoader, ());
  CHECK(m_estimator, ());
}

void TransitWorldGraph::GetEdgeList(astar::VertexData<Segment, RouteWeight> const & vertexData, bool isOutgoing,
                                    bool useRoutingOptions, bool useAccessConditional, SegmentEdgeListT & edges)
{
  auto const & segment = vertexData.m_vertex;
  auto & transitGraph = GetTransitGraph(segment.GetMwmId());

  if (TransitGraph::IsTransitSegment(segment))
  {
    for (auto const & s : transitGraph.GetFakeEdges(segment, isOutgoing))
    {
      auto const & from = isOutgoing ? segment : s;
      auto const & to = isOutgoing ? s : segment;
      edges.emplace_back(
          s, CalcSegmentWeight(to, EdgeEstimator::Purpose::Weight) + transitGraph.GetTransferPenalty(from, to));
    }

    Segment real;
    if (transitGraph.FindReal(segment, real))
    {
      bool const haveSameFront = GetJunction(segment, true /* front */) == GetJunction(real, true);
      bool const haveSameBack = GetJunction(segment, false /* front */) == GetJunction(real, false);
      astar::VertexData const data(real, vertexData.m_realDistance);
      if (isOutgoing && haveSameFront)
      {
        AddRealEdges(data, isOutgoing, useRoutingOptions, edges);
      }
      else if (!isOutgoing && haveSameBack)
      {
        // Ingoing real edges are priced by the segment they enter, the whole |real|, while this
        // vertex is only a part of it: rebase them to the part, keeping the crossing penalties.
        // The outgoing counterpart of this edge is priced the same way in the loop below.
        SegmentEdgeListT realEdges;
        GetIndexGraph(real.GetMwmId()).GetEdgeList(data, isOutgoing, useRoutingOptions, realEdges);
        RouteWeight const partDiff = CalcSegmentWeight(segment, EdgeEstimator::Purpose::Weight) -
                                     CalcSegmentWeight(real, EdgeEstimator::Purpose::Weight);
        for (auto const & edge : realEdges)
        {
          RouteWeight const weight = edge.GetWeight() + partDiff;
          ASSERT_GREATER_OR_EQUAL(weight.GetWeight(), 0.0, (segment, real));
          edges.emplace_back(edge.GetTarget(), weight);
        }
        // Twin edges are priced by the twin in its own mwm, so the rebase could go negative there;
        // keep the whole-segment price instead (a rare over-pricing on a transition segment).
        GetTwins(real, isOutgoing, useRoutingOptions, edges);
      }
    }

    GetTwins(segment, isOutgoing, useRoutingOptions, edges);
  }
  else
  {
    AddRealEdges(vertexData, isOutgoing, useRoutingOptions, edges);
  }

  SegmentEdgeListT fakeFromReal;
  for (auto const & edge : edges)
  {
    auto const & edgeSegment = edge.GetTarget();
    for (auto const & s : transitGraph.GetFake(edgeSegment))
    {
      bool const haveSameFront = GetJunction(edgeSegment, true /* front */) == GetJunction(s, true);
      bool const haveSameBack = GetJunction(edgeSegment, false /* front */) == GetJunction(s, false);
      // An edge is priced by the segment it enters plus the crossing penalties. Outgoing: |edge|
      // prices the whole |edgeSegment|, but only its part of real |s| is entered. Ingoing: |segment|
      // is entered and is already priced in |edge|.
      if (isOutgoing && haveSameBack)
      {
        fakeFromReal.emplace_back(s, edge.GetWeight() - CalcSegmentWeight(edgeSegment, EdgeEstimator::Purpose::Weight) +
                                         CalcSegmentWeight(s, EdgeEstimator::Purpose::Weight));
      }
      else if (!isOutgoing && haveSameFront)
      {
        fakeFromReal.emplace_back(s, edge.GetWeight());
      }
    }
  }
  edges.append(fakeFromReal.begin(), fakeFromReal.end());
}

void TransitWorldGraph::GetEdgeList(astar::VertexData<JointSegment, RouteWeight> const & parentVertexData,
                                    Segment const & segment, bool isOutgoing, bool useAccessConditional,
                                    JointEdgeListT & edges, WeightListT & parentWeights)
{
  CHECK(false, ("TransitWorldGraph does not support Joints mode."));
}

LatLonWithAltitude const & TransitWorldGraph::GetJunction(Segment const & segment, bool front)
{
  if (TransitGraph::IsTransitSegment(segment))
    return GetTransitGraph(segment.GetMwmId()).GetJunction(segment, front);

  return GetRealRoadGeometry(segment.GetMwmId(), segment.GetFeatureId()).GetJunction(segment.GetPointId(front));
}

ms::LatLon const & TransitWorldGraph::GetPoint(Segment const & segment, bool front)
{
  return GetJunction(segment, front).GetLatLon();
}

bool TransitWorldGraph::IsOneWay(NumMwmId mwmId, uint32_t featureId)
{
  if (TransitGraph::IsTransitFeature(featureId))
    return true;
  return GetRealRoadGeometry(mwmId, featureId).IsOneWay();
}

bool TransitWorldGraph::IsPassThroughAllowed(NumMwmId mwmId, uint32_t featureId)
{
  if (TransitGraph::IsTransitFeature(featureId))
    return true;
  return GetRealRoadGeometry(mwmId, featureId).IsPassThroughAllowed();
}

void TransitWorldGraph::ClearCachedGraphs()
{
  m_indexLoader->Clear();
  m_transitLoader->Clear();
}

RouteWeight TransitWorldGraph::HeuristicCostEstimate(ms::LatLon const & from, ms::LatLon const & to)
{
  return RouteWeight(m_estimator->CalcHeuristic(from, to));
}

RouteWeight TransitWorldGraph::CalcSegmentWeight(Segment const & segment, EdgeEstimator::Purpose purpose)
{
  if (TransitGraph::IsTransitSegment(segment))
  {
    TransitGraph & transitGraph = GetTransitGraph(segment.GetMwmId());

    Segment real;
    if (transitGraph.FindReal(segment, real))
    {
      // Part of a real road between a gate projection and the segment end. Price it as a fraction
      // of the real segment weight, like IndexGraphStarter does for start/finish endings; the
      // offroad speed would make a few metres of sidewalk cost minutes.
      double const partLen = ms::DistanceOnEarth(transitGraph.GetJunction(segment, false /* front */).GetLatLon(),
                                                 transitGraph.GetJunction(segment, true /* front */).GetLatLon());
      double const fullLen = ms::DistanceOnEarth(GetPoint(real, false /* front */), GetPoint(real, true /* front */));
      RouteWeight const weight = CalcSegmentWeight(real, purpose);
      return (fullLen == 0.0 ? 0.0 : partLen / fullLen) * weight;
    }

    return transitGraph.CalcSegmentWeight(segment);
  }

  return RouteWeight(m_estimator->CalcSegmentWeight(
      segment, GetRealRoadGeometry(segment.GetMwmId(), segment.GetFeatureId()), purpose));
}

RouteWeight TransitWorldGraph::CalcLeapWeight(ms::LatLon const & from, ms::LatLon const & to, NumMwmId mwmId) const
{
  return RouteWeight(m_estimator->CalcLeapWeight(from, to, mwmId));
}

RouteWeight TransitWorldGraph::CalcOffroadWeight(ms::LatLon const & from, ms::LatLon const & to,
                                                 EdgeEstimator::Purpose purpose) const
{
  return RouteWeight(m_estimator->CalcOffroad(from, to, purpose));
}

double TransitWorldGraph::CalculateETA(Segment const & from, Segment const & to, time_t arrivalTime)
{
  if (TransitGraph::IsTransitSegment(from))
    return CalcSegmentWeight(to, EdgeEstimator::Purpose::ETA).GetWeight();

  if (TransitGraph::IsTransitSegment(to))
    return CalcSegmentWeight(to, EdgeEstimator::Purpose::ETA).GetWeight();

  if (from.GetMwmId() != to.GetMwmId())
  {
    return m_estimator->CalcSegmentWeight(to, GetRealRoadGeometry(to.GetMwmId(), to.GetFeatureId()),
                                          EdgeEstimator::Purpose::ETA);
  }

  auto & indexGraph = m_indexLoader->GetIndexGraph(from.GetMwmId());
  return indexGraph
      .CalculateEdgeWeight(EdgeEstimator::Purpose::ETA, true /* isOutgoing */, from, to, RouteWeight(arrivalTime))
      .GetWeight();
}

double TransitWorldGraph::CalculateETAWithoutPenalty(Segment const & segment)
{
  if (TransitGraph::IsTransitSegment(segment))
    return CalcSegmentWeight(segment, EdgeEstimator::Purpose::ETA).GetWeight();

  return m_estimator->CalcSegmentWeight(segment, GetRealRoadGeometry(segment.GetMwmId(), segment.GetFeatureId()),
                                        EdgeEstimator::Purpose::ETA);
}

std::unique_ptr<TransitInfo> TransitWorldGraph::GetTransitInfo(Segment const & segment)
{
  if (!TransitGraph::IsTransitSegment(segment))
    return {};

  auto & transitGraph = GetTransitGraph(segment.GetMwmId());
  if (auto const * gate = transitGraph.FindGate(segment))
    return std::make_unique<TransitInfo>(*gate);

  if (auto const * edge = transitGraph.FindEdge(segment))
    return std::make_unique<TransitInfo>(*edge);

  // Fake segment between pedestrian feature and gate.
  return {};
}

void TransitWorldGraph::GetGatesNear(NumMwmId mwmId, m2::PointD const & point, double radiusM, bool isEnter,
                                     GateAccessesT & out)
{
  GetTransitGraph(mwmId).GetGatesNear(point, radiusM, isEnter, out);
}

void TransitWorldGraph::GetTwinsInner(Segment const & segment, bool isOutgoing, TwinSegmentsListT & twins)
{
  if (m_mode == WorldGraphMode::SingleMwm || !m_crossMwmGraph || !m_crossMwmGraph->IsTransition(segment, isOutgoing))
    return;
  m_crossMwmGraph->GetTwins(segment, isOutgoing, twins);
}

RoadGeometry const & TransitWorldGraph::GetRealRoadGeometry(NumMwmId mwmId, uint32_t featureId)
{
  CHECK(!TransitGraph::IsTransitFeature(featureId), ("GetRealRoadGeometry not designed for transit."));
  return m_indexLoader->GetIndexGraph(mwmId).GetRoadGeometry(featureId);
}

void TransitWorldGraph::AddRealEdges(astar::VertexData<Segment, RouteWeight> const & vertexData, bool isOutgoing,
                                     bool useRoutingOptions, SegmentEdgeListT & edges)
{
  auto const & segment = vertexData.m_vertex;
  auto & indexGraph = GetIndexGraph(segment.GetMwmId());
  indexGraph.GetEdgeList(vertexData, isOutgoing, useRoutingOptions, edges);
  GetTwins(segment, isOutgoing, useRoutingOptions, edges);
}

TransitGraph & TransitWorldGraph::GetTransitGraph(NumMwmId mwmId)
{
  auto & indexGraph = GetIndexGraph(mwmId);
  return m_transitLoader->GetTransitGraph(mwmId, indexGraph);
}
}  // namespace routing
