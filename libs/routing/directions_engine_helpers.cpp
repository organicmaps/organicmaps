#include "routing/directions_engine_helpers.hpp"

#include "geometry/mercator.hpp"

namespace routing
{
bool AdjacentEdges::IsAlmostEqual(AdjacentEdges const & rhs) const
{
  return m_outgoingTurns.IsAlmostEqual(rhs.m_outgoingTurns) && m_ingoingTurnsCount == rhs.m_ingoingTurnsCount;
}

RoutingEngineResult::RoutingEngineResult(IRoadGraph::EdgeVector const & routeEdges,
                                         AdjacentEdgesMap const & adjacentEdges,
                                         TUnpackedPathSegments const & pathSegments)
  : m_routeEdges(routeEdges)
  , m_adjacentEdges(adjacentEdges)
  , m_pathSegments(pathSegments)
  , m_routeLength(0)
{
  for (auto const & edge : routeEdges)
    m_routeLength += mercator::DistanceOnEarth(edge.GetStartJunction().GetPoint(), edge.GetEndJunction().GetPoint());
}

void RoutingEngineResult::GetPossibleTurns(SegmentRange const & segmentRange, m2::PointD const & junctionPoint,
                                           size_t & ingoingCount, turns::TurnCandidates & outgoingTurns) const
{
  CHECK(!segmentRange.IsEmpty(),
        ("SegmentRange presents a fake feature.", "junctionPoint:", mercator::ToLatLon(junctionPoint)));

  ingoingCount = 0;
  outgoingTurns.candidates.clear();

  auto const adjacentEdges = m_adjacentEdges.find(segmentRange);
  if (adjacentEdges == m_adjacentEdges.cend())
  {
    ASSERT(false, (segmentRange));
    return;
  }

  ingoingCount = adjacentEdges->second.m_ingoingTurnsCount;
  outgoingTurns = adjacentEdges->second.m_outgoingTurns;
}

geometry::PointWithAltitude RoutingEngineResult::GetStartPoint() const
{
  CHECK(!m_routeEdges.empty(), ());
  return m_routeEdges.front().GetStartJunction();
}

geometry::PointWithAltitude RoutingEngineResult::GetEndPoint() const
{
  CHECK(!m_routeEdges.empty(), ());
  return m_routeEdges.back().GetEndJunction();
}

bool IsJoint(IRoadGraph::EdgeListT const & ingoingEdges, IRoadGraph::EdgeListT const & outgoingEdges,
             Edge const & ingoingRouteEdge, Edge const & outgoingRouteEdge)
{
  // When feature id is changed at a junction this junction should be considered as a joint.
  //
  // If a feature id is not changed at a junction but the junction has some ingoing or outgoing
  // edges with different feature ids, the junction should be considered as a joint.
  //
  // If a feature id is not changed at a junction and all ingoing and outgoing edges of the junction
  // has the same feature id, the junction still may be considered as a joint. It happens in case of
  // self intersected features. For example:
  //            *--Seg3--*
  //            |        |
  //          Seg4      Seg2
  //            |        |
  //   *--Seg0--*--Seg1--*
  // The common point of segments 0, 1 and 4 should be considered as a joint.

  if (ingoingRouteEdge.GetFeatureId() != outgoingRouteEdge.GetFeatureId())
    return true;

  FeatureID const & featureId = ingoingRouteEdge.GetFeatureId();
  uint32_t const segOut = outgoingRouteEdge.GetSegId();
  for (Edge const & e : ingoingEdges)
    if (e.GetFeatureId() != featureId || abs(static_cast<int32_t>(segOut - e.GetSegId())) != 1)
      return true;

  uint32_t const segIn = ingoingRouteEdge.GetSegId();
  for (Edge const & e : outgoingEdges)
  {
    // It's necessary to compare segments for cases when |featureId| is a loop.
    if (e.GetFeatureId() != featureId || abs(static_cast<int32_t>(segIn - e.GetSegId())) != 1)
      return true;
  }
  return false;
}
}  // namespace routing
