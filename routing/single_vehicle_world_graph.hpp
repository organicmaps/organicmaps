#pragma once

#include "routing/cross_mwm_graph.hpp"
#include "routing/edge_estimator.hpp"
#include "routing/geometry.hpp"
#include "routing/index_graph.hpp"
#include "routing/index_graph_loader.hpp"
#include "routing/joint_segment.hpp"
#include "routing/road_graph.hpp"
#include "routing/route.hpp"
#include "routing/segment.hpp"
#include "routing/transit_info.hpp"
#include "routing/world_graph.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "geometry/point2d.hpp"

#include <functional>
#include <map>
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
  // @{
  ~SingleVehicleWorldGraph() override = default;

  void GetEdgeList(Segment const & segment, bool isOutgoing, bool useRoutingOptions,
                   std::vector<SegmentEdge> & edges) override;

  void GetEdgeList(JointSegment const & parentJoint, Segment const & parent, bool isOutgoing,
                   std::vector<JointEdge> & jointEdges, std::vector<RouteWeight> & parentWeights) override;

  bool CheckLength(RouteWeight const &, double) const override { return true; }

  Junction const & GetJunction(Segment const & segment, bool front) override;
  m2::PointD const & GetPoint(Segment const & segment, bool front) override;

  bool IsOneWay(NumMwmId mwmId, uint32_t featureId) override;
  bool IsPassThroughAllowed(NumMwmId mwmId, uint32_t featureId) override;
  void ClearCachedGraphs() override { m_loader->Clear(); }

  void SetMode(WorldGraphMode mode) override { m_mode = mode; }
  WorldGraphMode GetMode() const override { return m_mode; }

  void GetOutgoingEdgesList(Segment const & segment, std::vector<SegmentEdge> & edges) override;
  void GetIngoingEdgesList(Segment const & segment, std::vector<SegmentEdge> & edges) override;

  RouteWeight HeuristicCostEstimate(Segment const & from, Segment const & to) override;
  RouteWeight HeuristicCostEstimate(m2::PointD const & from, m2::PointD const & to) override;
  RouteWeight HeuristicCostEstimate(Segment const & from, m2::PointD const & to) override;

  RouteWeight CalcSegmentWeight(Segment const & segment) override;
  RouteWeight CalcLeapWeight(m2::PointD const & from, m2::PointD const & to) const override;
  RouteWeight CalcOffroadWeight(m2::PointD const & from, m2::PointD const & to) const override;
  double CalcSegmentETA(Segment const & segment) override;

  std::vector<Segment> const & GetTransitions(NumMwmId numMwmId, bool isEnter) override;

  void SetRoutingOptions(RoutingOptions routingOptions) override { m_avoidRoutingOptions = routingOptions; }
  /// \returns true if feature, associated with segment satisfies users conditions.
  bool IsRoutingOptionsGood(Segment const & segment) override;
  RoutingOptions GetRoutingOptions(Segment const & segment) override;

  std::unique_ptr<TransitInfo> GetTransitInfo(Segment const & segment) override;
  std::vector<RouteSegment::SpeedCamera> GetSpeedCamInfo(Segment const & segment) override;

  IndexGraph & GetIndexGraph(NumMwmId numMwmId) override
  {
    return m_loader->GetIndexGraph(numMwmId);
  }

  void SetAStarParents(bool forward, ParentSegments & parents) override;
  void SetAStarParents(bool forward, ParentJoints & parents) override;

  bool AreWavesConnectible(ParentSegments & forwardParents, Segment const & commonVertex,
                           ParentSegments & backwardParents,
                           std::function<uint32_t(Segment const &)> && fakeFeatureConverter) override;
  bool AreWavesConnectible(ParentJoints & forwardParents, JointSegment const & commonVertex,
                           ParentJoints & backwardParents,
                           std::function<uint32_t(JointSegment const &)> && fakeFeatureConverter) override;
  // @}

  // This method should be used for tests only
  IndexGraph & GetIndexGraphForTests(NumMwmId numMwmId)
  {
    return m_loader->GetIndexGraph(numMwmId);
  }

private:
  /// \brief Get parents' featureIds of |commonVertex| from forward AStar wave and join them with
  ///        parents' featureIds from backward wave.
  /// \return The result chain of fetureIds are used to find restrictions on it and understand whether
  ///         waves are connectable or not.
  template <typename VertexType>
  bool AreWavesConnectibleImpl(std::map<VertexType, VertexType> const & forwardParents,
                               VertexType const & commonVertex,
                               std::map<VertexType, VertexType> const & backwardParents,
                               std::function<uint32_t(VertexType const &)> && fakeFeatureConverter);

  // Retrieves the same |jointEdges|, but into others mwms.
  // If they are cross mwm edges, of course.
  void CheckAndProcessTransitFeatures(Segment const & parent,
                                      std::vector<JointEdge> & jointEdges,
                                      std::vector<RouteWeight> & parentWeights,
                                      bool isOutgoing);
  // WorldGraph overrides:
  void GetTwinsInner(Segment const & s, bool isOutgoing, std::vector<Segment> & twins) override;

  RoadGeometry const & GetRoadGeometry(NumMwmId mwmId, uint32_t featureId);

  std::unique_ptr<CrossMwmGraph> m_crossMwmGraph;
  std::unique_ptr<IndexGraphLoader> m_loader;
  std::shared_ptr<EdgeEstimator> m_estimator;
  RoutingOptions m_avoidRoutingOptions = RoutingOptions();
  WorldGraphMode m_mode = WorldGraphMode::NoLeaps;

  template <typename Vertex>
  struct AStarParents
  {
    using ParentType = std::map<Vertex, Vertex>;
    static ParentType kEmpty;
    ParentType * forward = &kEmpty;
    ParentType * backward = &kEmpty;
  };

  AStarParents<Segment> m_parentsForSegments;
  AStarParents<JointSegment> m_parentsForJoints;
};
}  // namespace routing
