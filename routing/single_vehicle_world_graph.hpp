#pragma once

#include "routing/cross_mwm_graph.hpp"
#include "routing/edge_estimator.hpp"
#include "routing/geometry.hpp"
#include "routing/index_graph.hpp"
#include "routing/index_graph_loader.hpp"
#include "routing/joint_segment.hpp"
#include "routing/mwm_hierarchy_handler.hpp"
#include "routing/route.hpp"
#include "routing/segment.hpp"
#include "routing/world_graph.hpp"

#include "routing_common/num_mwm_id.hpp"

#include <map>
#include <memory>
#include <vector>

namespace routing
{
class SingleVehicleWorldGraph final : public WorldGraph
{
public:
  SingleVehicleWorldGraph(std::unique_ptr<CrossMwmGraph> crossMwmGraph, std::unique_ptr<IndexGraphLoader> loader,
                          std::shared_ptr<EdgeEstimator> estimator, MwmHierarchyHandler && hierarchyHandler);

  // WorldGraph overrides:
  // @{
  ~SingleVehicleWorldGraph() override = default;

  using WorldGraph::GetEdgeList;

  void GetEdgeList(astar::VertexData<Segment, RouteWeight> const & vertexData, bool isOutgoing, bool useRoutingOptions,
                   bool useAccessConditional, SegmentEdgeListT & edges) override;

  void GetEdgeList(astar::VertexData<JointSegment, RouteWeight> const & parentVertexData, Segment const & parent,
                   bool isOutgoing, bool useAccessConditional, JointEdgeListT & jointEdges,
                   WeightListT & parentWeights) override;

  bool CheckLength(RouteWeight const &, double) const override { return true; }

  LatLonWithAltitude const & GetJunction(Segment const & segment, bool front) override;
  ms::LatLon const & GetPoint(Segment const & segment, bool front) override;

  bool IsOneWay(NumMwmId mwmId, uint32_t featureId) override;
  bool IsPassThroughAllowed(NumMwmId mwmId, uint32_t featureId) override;
  void ClearCachedGraphs() override { m_loader->Clear(); }

  void SetMode(WorldGraphMode mode) override { m_mode = mode; }
  WorldGraphMode GetMode() const override { return m_mode; }

  RouteWeight HeuristicCostEstimate(ms::LatLon const & from, ms::LatLon const & to) override;

  RouteWeight CalcSegmentWeight(Segment const & segment, EdgeEstimator::Purpose purpose) override;
  RouteWeight CalcLeapWeight(ms::LatLon const & from, ms::LatLon const & to, NumMwmId mwmId) const override;
  RouteWeight CalcOffroadWeight(ms::LatLon const & from, ms::LatLon const & to,
                                EdgeEstimator::Purpose purpose) const override;
  double CalculateETA(Segment const & from, Segment const & to) override;
  double CalculateETAWithoutPenalty(Segment const & segment) override;

  void ForEachTransition(NumMwmId numMwmId, bool isEnter, TransitionFnT const & fn) override;

  void SetRoutingOptions(RoutingOptions routingOptions) override { m_avoidRoutingOptions = routingOptions; }
  /// \returns true if feature, associated with segment satisfies users conditions.
  bool IsRoutingOptionsGood(Segment const & segment) override;
  RoutingOptions GetRoutingOptions(Segment const & segment) override;

  std::vector<RouteSegment::SpeedCamera> GetSpeedCamInfo(Segment const & segment) override;
  SpeedInUnits GetSpeedLimit(Segment const & segment) override;

  IndexGraph & GetIndexGraph(NumMwmId numMwmId) override { return m_loader->GetIndexGraph(numMwmId); }

  void SetAStarParents(bool forward, Parents<Segment> & parents) override;
  void SetAStarParents(bool forward, Parents<JointSegment> & parents) override;
  void DropAStarParents() override;

  bool AreWavesConnectible(Parents<Segment> & forwardParents, Segment const & commonVertex,
                           Parents<Segment> & backwardParents) override;
  bool AreWavesConnectible(Parents<JointSegment> & forwardParents, JointSegment const & commonVertex,
                           Parents<JointSegment> & backwardParents,
                           FakeConverterT const & fakeFeatureConverter) override;
  // @}

  // This method should be used for tests only
  IndexGraph & GetIndexGraphForTests(NumMwmId numMwmId) { return m_loader->GetIndexGraph(numMwmId); }

  CrossMwmGraph & GetCrossMwmGraph() override { return *m_crossMwmGraph; }

private:
  /// \brief Get parents' featureIds of |commonVertex| from forward AStar wave and join them with
  ///        parents' featureIds from backward wave.
  /// \return The result chain of fetureIds are used to find restrictions on it and understand whether
  ///         waves are connectable or not.
  template <typename VertexType, class ConverterT>
  bool AreWavesConnectibleImpl(Parents<VertexType> const & forwardParents, VertexType const & commonVertex,
                               Parents<VertexType> const & backwardParents, ConverterT const & fakeConverter);

  // Retrieves the same |jointEdges|, but into others mwms.
  // If they are cross mwm edges, of course.
  void CheckAndProcessTransitFeatures(Segment const & parent, JointEdgeListT & jointEdges, WeightListT & parentWeights,
                                      bool isOutgoing);
  /// @name WorldGraph overrides.
  /// @{
  void GetTwinsInner(Segment const & s, bool isOutgoing, std::vector<Segment> & twins) override;
  RouteWeight GetCrossBorderPenalty(NumMwmId mwmId1, NumMwmId mwmId2) override;
  /// @}

  RoadGeometry const & GetRoadGeometry(NumMwmId mwmId, uint32_t featureId);

  std::unique_ptr<CrossMwmGraph> m_crossMwmGraph;
  std::unique_ptr<IndexGraphLoader> m_loader;
  std::shared_ptr<EdgeEstimator> m_estimator;
  RoutingOptions m_avoidRoutingOptions;
  WorldGraphMode m_mode = WorldGraphMode::NoLeaps;

  template <typename Vertex>
  struct AStarParents
  {
    using ParentType = Parents<Vertex>;
    static ParentType kEmpty;
    ParentType * forward = &kEmpty;
    ParentType * backward = &kEmpty;
  };

  AStarParents<Segment> m_parentsForSegments;
  AStarParents<JointSegment> m_parentsForJoints;

  MwmHierarchyHandler m_hierarchyHandler;
};
}  // namespace routing
