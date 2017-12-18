#pragma once

#include "routing/cross_mwm_graph.hpp"
#include "routing/edge_estimator.hpp"
#include "routing/geometry.hpp"
#include "routing/index_graph.hpp"
#include "routing/index_graph_loader.hpp"
#include "routing/road_graph.hpp"
#include "routing/segment.hpp"
#include "routing/transit_info.hpp"
#include "routing/world_graph.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "geometry/point2d.hpp"

#include <memory>
#include <vector>

namespace routing
{
class SingleVehicleWorldGraph final : public WorldGraph
{
public:
  SingleVehicleWorldGraph(std::unique_ptr<CrossMwmGraph> crossMwmGraph,
                          std::unique_ptr<IndexGraphLoader> loader,
                          std::shared_ptr<EdgeEstimator> estimator);

  // WorldGraph overrides:
  ~SingleVehicleWorldGraph() override = default;

  void GetEdgeList(Segment const & segment, bool isOutgoing,
                   std::vector<SegmentEdge> & edges) override;
  bool CheckLength(RouteWeight const &, double) const override { return true; }
  Junction const & GetJunction(Segment const & segment, bool front) override;
  m2::PointD const & GetPoint(Segment const & segment, bool front) override;
  bool IsOneWay(NumMwmId mwmId, uint32_t featureId) override;
  bool IsPassThroughAllowed(NumMwmId mwmId, uint32_t featureId) override;
  void ClearCachedGraphs() override { m_loader->Clear(); }
  void SetMode(Mode mode) override { m_mode = mode; }
  Mode GetMode() const override { return m_mode; }
  void GetOutgoingEdgesList(Segment const & segment, std::vector<SegmentEdge> & edges) override;
  void GetIngoingEdgesList(Segment const & segment, std::vector<SegmentEdge> & edges) override;
  RouteWeight HeuristicCostEstimate(Segment const & from, Segment const & to) override;
  RouteWeight HeuristicCostEstimate(m2::PointD const & from, m2::PointD const & to) override;
  RouteWeight CalcSegmentWeight(Segment const & segment) override;
  RouteWeight CalcLeapWeight(m2::PointD const & from, m2::PointD const & to) const override;
  RouteWeight CalcOffroadWeight(m2::PointD const & from, m2::PointD const & to) const override;
  bool LeapIsAllowed(NumMwmId mwmId) const override;
  std::vector<Segment> const & GetTransitions(NumMwmId numMwmId, bool isEnter) override;
  std::unique_ptr<TransitInfo> GetTransitInfo(Segment const & segment) override;

  // This method should be used for tests only
  IndexGraph & GetIndexGraphForTests(NumMwmId numMwmId)
  {
    return m_loader->GetIndexGraph(numMwmId);
  }

private:
  // WorldGraph overrides:
  void GetTwinsInner(Segment const & s, bool isOutgoing, std::vector<Segment> & twins) override;

  RoadGeometry const & GetRoadGeometry(NumMwmId mwmId, uint32_t featureId);

  std::unique_ptr<CrossMwmGraph> m_crossMwmGraph;
  std::unique_ptr<IndexGraphLoader> m_loader;
  std::shared_ptr<EdgeEstimator> m_estimator;
  Mode m_mode = Mode::NoLeaps;
};
}  // namespace routing
