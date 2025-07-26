#pragma once

#include "routing/base/astar_graph.hpp"
#include "routing/base/astar_vertex_data.hpp"
#include "routing/mwm_hierarchy_handler.hpp"
#include "routing/route_weight.hpp"
#include "routing/segment.hpp"

#include "geometry/latlon.hpp"

#include <vector>

namespace routing
{
class IndexGraphStarter;

class LeapsGraph : public AStarGraph<Segment, SegmentEdge, RouteWeight>
{
public:
  LeapsGraph(IndexGraphStarter & starter, MwmHierarchyHandler && hierarchyHandler);

  // AStarGraph overrides:
  // @{
  void GetOutgoingEdgesList(astar::VertexData<Vertex, Weight> const & vertexData, EdgeListT & edges) override;
  void GetIngoingEdgesList(astar::VertexData<Vertex, Weight> const & vertexData, EdgeListT & edges) override;
  RouteWeight HeuristicCostEstimate(Segment const & from, Segment const & to) override;
  RouteWeight GetAStarWeightEpsilon() override;
  // @}

  Segment const & GetStartSegment() const { return m_startSegment; }
  Segment const & GetFinishSegment() const { return m_finishSegment; }
  ms::LatLon const & GetPoint(Segment const & segment, bool front) const;

  RouteWeight CalcMiddleCrossMwmWeight(std::vector<Segment> const & path);

private:
  void GetEdgesList(Segment const & segment, bool isOutgoing, EdgeListT & edges);

  void GetEdgesListFromStart(EdgeListT & edges) const;
  void GetEdgesListToFinish(EdgeListT & edges) const;

private:
  ms::LatLon m_startPoint;
  ms::LatLon m_finishPoint;

  Segment m_startSegment;
  Segment m_finishSegment;

  IndexGraphStarter & m_starter;

  MwmHierarchyHandler m_hierarchyHandler;
};
}  // namespace routing
