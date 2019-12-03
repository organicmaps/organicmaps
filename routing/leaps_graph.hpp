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

  // AStarGraph overrides:
  // @{
  void GetOutgoingEdgesList(Segment const & segment, std::vector<SegmentEdge> & edges) override;
  void GetIngoingEdgesList(Segment const & segment, std::vector<SegmentEdge> & edges) override;
  RouteWeight HeuristicCostEstimate(Segment const & from, Segment const & to) override;
  RouteWeight GetAStarWeightEpsilon() override;
  // @}

  m2::PointD const & GetPoint(Segment const & segment, bool front);
  Segment const & GetStartSegment() const;
  Segment const & GetFinishSegment() const;

private:
  void GetEdgesList(Segment const & segment, bool isOutgoing, std::vector<SegmentEdge> & edges);

  void GetEdgesListFromStart(Segment const & segment, bool isOutgoing,
                             std::vector<SegmentEdge> & edges);
  void GetEdgesListToFinish(Segment const & segment, bool isOutgoing,
                             std::vector<SegmentEdge> & edges);

  m2::PointD m_startPoint;
  m2::PointD m_finishPoint;

  Segment m_startSegment;
  Segment m_finishSegment;

  IndexGraphStarter & m_starter;
};
}  // namespace routing
