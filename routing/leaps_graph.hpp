#pragma once

#include "routing/base/astar_graph.hpp"

#include "routing/index_graph_starter.hpp"
#include "routing/route_weight.hpp"
#include "routing/segment.hpp"

#include "geometry/point2d.hpp"

#include <vector>

namespace routing
{
class LeapsGraph : public AStarGraph<Segment, SegmentEdge, RouteWeight>
{
public:
  explicit LeapsGraph(IndexGraphStarter & starter);

  // AStarGraph overridings:
  // @{
  void GetOutgoingEdgesList(Segment const & segment, std::vector<SegmentEdge> & edges) override;
  void GetIngoingEdgesList(Segment const & segment, std::vector<SegmentEdge> & edges) override;
  RouteWeight HeuristicCostEstimate(Segment const & from, Segment const & to) override;
  RouteWeight GetAStarWeightEpsilon() override;
  // @}

  m2::PointD const & GetPoint(Segment const & segment, bool front) const;

private:
  void GetEdgesList(Segment const & segment, bool isOutgoing, std::vector<SegmentEdge> & edges);
  void GetEdgesListForStart(Segment const & segment, bool isOutgoing,
                            std::vector<SegmentEdge> & edges);
  void GetEdgesListForFinish(Segment const & segment, bool isOutgoing,
                             std::vector<SegmentEdge> & edges);

  IndexGraphStarter & m_starter;
};
}  // namespace routing
