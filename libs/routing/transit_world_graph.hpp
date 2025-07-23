#pragma once

#include "routing/base/astar_vertex_data.hpp"

#include "routing/cross_mwm_graph.hpp"
#include "routing/edge_estimator.hpp"
#include "routing/geometry.hpp"
#include "routing/index_graph_loader.hpp"
#include "routing/segment.hpp"
#include "routing/transit_graph_loader.hpp"
#include "routing/transit_info.hpp"
#include "routing/world_graph.hpp"

#include "transit/transit_types.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "geometry/latlon.hpp"

#include <memory>
#include <vector>

namespace routing
{
// WorldGraph for transit + pedestrian routing
class TransitWorldGraph final : public WorldGraph
{
public:
  TransitWorldGraph(std::unique_ptr<CrossMwmGraph> crossMwmGraph, std::unique_ptr<IndexGraphLoader> indexLoader,
                    std::unique_ptr<TransitGraphLoader> transitLoader, std::shared_ptr<EdgeEstimator> estimator);

  // WorldGraph overrides:
  ~TransitWorldGraph() override = default;

  using WorldGraph::GetEdgeList;

  void GetEdgeList(astar::VertexData<Segment, RouteWeight> const & vertexData, bool isOutgoing, bool useRoutingOptions,
                   bool useAccessConditional, SegmentEdgeListT & edges) override;
  // Dummy method which shouldn't be called.
  void GetEdgeList(astar::VertexData<JointSegment, RouteWeight> const & parentVertexData, Segment const & segment,
                   bool isOutgoing, bool useAccessConditional, JointEdgeListT & edges,
                   WeightListT & parentWeights) override;

  bool CheckLength(RouteWeight const & weight, double startToFinishDistanceM) const override
  {
    return weight.GetWeight() - weight.GetTransitTime() <= MaxPedestrianTimeSec(startToFinishDistanceM);
  }
  LatLonWithAltitude const & GetJunction(Segment const & segment, bool front) override;
  ms::LatLon const & GetPoint(Segment const & segment, bool front) override;
  // All transit features are oneway.
  bool IsOneWay(NumMwmId mwmId, uint32_t featureId) override;
  // All transit features are allowed for through passage.
  bool IsPassThroughAllowed(NumMwmId mwmId, uint32_t featureId) override;
  void ClearCachedGraphs() override;
  void SetMode(WorldGraphMode mode) override { m_mode = mode; }
  WorldGraphMode GetMode() const override { return m_mode; }

  RouteWeight HeuristicCostEstimate(ms::LatLon const & from, ms::LatLon const & to) override;

  RouteWeight CalcSegmentWeight(Segment const & segment, EdgeEstimator::Purpose purpose) override;
  RouteWeight CalcLeapWeight(ms::LatLon const & from, ms::LatLon const & to, NumMwmId mwmId) const override;
  RouteWeight CalcOffroadWeight(ms::LatLon const & from, ms::LatLon const & to,
                                EdgeEstimator::Purpose purpose) const override;
  double CalculateETA(Segment const & from, Segment const & to) override;
  double CalculateETAWithoutPenalty(Segment const & segment) override;

  std::unique_ptr<TransitInfo> GetTransitInfo(Segment const & segment) override;

  IndexGraph & GetIndexGraph(NumMwmId numMwmId) override { return m_indexLoader->GetIndexGraph(numMwmId); }

private:
  // WorldGraph overrides:
  void GetTwinsInner(Segment const & s, bool isOutgoing, std::vector<Segment> & twins) override;

  static double MaxPedestrianTimeSec(double startToFinishDistanceM)
  {
    // @todo(bykoianko) test and adjust constants.
    // 50 min + 3 additional minutes per 1 km for now.
    return 50 * 60 + (startToFinishDistanceM / 1000) * 3 * 60;
  }

  RoadGeometry const & GetRealRoadGeometry(NumMwmId mwmId, uint32_t featureId);
  void AddRealEdges(astar::VertexData<Segment, RouteWeight> const & vertexData, bool isOutgoing, bool useRoutingOptions,
                    SegmentEdgeListT & edges);
  TransitGraph & GetTransitGraph(NumMwmId mwmId);

  std::unique_ptr<CrossMwmGraph> m_crossMwmGraph;
  std::unique_ptr<IndexGraphLoader> m_indexLoader;
  std::unique_ptr<TransitGraphLoader> m_transitLoader;
  std::shared_ptr<EdgeEstimator> m_estimator;
  WorldGraphMode m_mode = WorldGraphMode::NoLeaps;
};
}  // namespace routing
