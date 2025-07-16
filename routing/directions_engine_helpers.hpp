#pragma once

#include "routing/loaded_path_segment.hpp"
#include "routing/road_graph.hpp"
#include "routing/routing_result_graph.hpp"
#include "routing/turn_candidate.hpp"
#include "routing/turns.hpp"

#include "geometry/point2d.hpp"

namespace routing
{
struct AdjacentEdges
{
  explicit AdjacentEdges(size_t ingoingTurnsCount = 0) : m_ingoingTurnsCount(ingoingTurnsCount) {}
  bool IsAlmostEqual(AdjacentEdges const & rhs) const;

  turns::TurnCandidates m_outgoingTurns;
  size_t m_ingoingTurnsCount;
};

using AdjacentEdgesMap = std::map<SegmentRange, AdjacentEdges>;

class RoutingEngineResult : public turns::IRoutingResult
{
public:
  RoutingEngineResult(IRoadGraph::EdgeVector const & routeEdges, AdjacentEdgesMap const & adjacentEdges,
                      TUnpackedPathSegments const & pathSegments);

  // turns::IRoutingResult overrides:
  TUnpackedPathSegments const & GetSegments() const override { return m_pathSegments; }

  void GetPossibleTurns(SegmentRange const & segmentRange, m2::PointD const & junctionPoint, size_t & ingoingCount,
                        turns::TurnCandidates & outgoingTurns) const override;

  double GetPathLength() const override { return m_routeLength; }
  geometry::PointWithAltitude GetStartPoint() const override;
  geometry::PointWithAltitude GetEndPoint() const override;

private:
  IRoadGraph::EdgeVector const & m_routeEdges;
  AdjacentEdgesMap const & m_adjacentEdges;
  TUnpackedPathSegments const & m_pathSegments;
  double m_routeLength;
};

/// \brief This method should be called for an internal junction of the route with corresponding
/// |ingoingEdges|, |outgoingEdges|, |ingoingRouteEdge| and |outgoingRouteEdge|.
/// \returns false if the junction is an internal point of feature segment and can be considered as
/// a part of LoadedPathSegment and returns true if the junction should be considered as a beginning
/// of a new LoadedPathSegment.
bool IsJoint(IRoadGraph::EdgeListT const & ingoingEdges, IRoadGraph::EdgeListT const & outgoingEdges,
             Edge const & ingoingRouteEdge, Edge const & outgoingRouteEdge);
}  // namespace routing
