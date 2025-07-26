#pragma once

#include "routing/base/astar_vertex_data.hpp"

#include "routing/edge_estimator.hpp"
#include "routing/index_graph.hpp"
#include "routing/joint_segment.hpp"
#include "routing/latlon_with_altitude.hpp"
#include "routing/route.hpp"
#include "routing/routing_options.hpp"
#include "routing/segment.hpp"
#include "routing/transit_info.hpp"

#include "routing_common/num_mwm_id.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace routing
{
class CrossMwmGraph;

enum class WorldGraphMode
{
  LeapsOnly,       // Mode for building a cross mwm route containing only leaps. In case of start and
                   // finish they (start and finish) will be connected with all transition segments of
                   // their mwm with leap (fake) edges.
  NoLeaps,         // Mode for building route and getting outgoing/ingoing edges without leaps at all.
  SingleMwm,       // Mode for building route and getting outgoing/ingoing edges within mwm source
                   // segment belongs to.
  Joints,          // Mode for building route with jumps between Joints.
  JointSingleMwm,  // Like |SingleMwm|, but in |Joints| mode.

  Undefined  // Default mode, until initialization.
};

class WorldGraph
{
public:
  template <typename VertexType>
  using Parents = IndexGraph::Parents<VertexType>;

  using JointEdgeListT = IndexGraph::JointEdgeListT;
  using SegmentEdgeListT = IndexGraph::SegmentEdgeListT;
  using WeightListT = IndexGraph::WeightListT;

  virtual ~WorldGraph() = default;

  virtual void GetEdgeList(astar::VertexData<Segment, RouteWeight> const & vertexData, bool isOutgoing,
                           bool useRoutingOptions, bool useAccessConditional, SegmentEdgeListT & edges) = 0;
  virtual void GetEdgeList(astar::VertexData<JointSegment, RouteWeight> const & vertexData, Segment const & segment,
                           bool isOutgoing, bool useAccessConditional, JointEdgeListT & edges,
                           WeightListT & parentWeights) = 0;

  bool IsRegionsGraphMode() const { return m_isRegionsGraphMode; }

  void SetRegionsGraphMode(bool isRegionsGraphMode) { m_isRegionsGraphMode = isRegionsGraphMode; }

  void GetEdgeList(Segment const & vertex, bool isOutgoing, bool useRoutingOptions, SegmentEdgeListT & edges);

  // Checks whether path length meets restrictions. Restrictions may depend on the distance from
  // start to finish of the route.
  virtual bool CheckLength(RouteWeight const & weight, double startToFinishDistanceM) const = 0;

  virtual LatLonWithAltitude const & GetJunction(Segment const & segment, bool front) = 0;
  virtual ms::LatLon const & GetPoint(Segment const & segment, bool front) = 0;
  virtual bool IsOneWay(NumMwmId mwmId, uint32_t featureId) = 0;

  // Checks whether feature is allowed for through passage.
  virtual bool IsPassThroughAllowed(NumMwmId mwmId, uint32_t featureId) = 0;

  // Clear memory used by loaded graphs.
  virtual void ClearCachedGraphs() = 0;
  virtual void SetMode(WorldGraphMode mode) = 0;
  virtual WorldGraphMode GetMode() const = 0;

  virtual RouteWeight HeuristicCostEstimate(ms::LatLon const & from, ms::LatLon const & to) = 0;

  virtual RouteWeight CalcSegmentWeight(Segment const & segment, EdgeEstimator::Purpose purpose) = 0;

  virtual RouteWeight CalcLeapWeight(ms::LatLon const & from, ms::LatLon const & to, NumMwmId mwmId) const = 0;

  virtual RouteWeight CalcOffroadWeight(ms::LatLon const & from, ms::LatLon const & to,
                                        EdgeEstimator::Purpose purpose) const = 0;

  virtual double CalculateETA(Segment const & from, Segment const & to) = 0;
  virtual double CalculateETAWithoutPenalty(Segment const & segment) = 0;

  using TransitionFnT = std::function<void(Segment const &)>;
  virtual void ForEachTransition(NumMwmId numMwmId, bool isEnter, TransitionFnT const & fn);

  virtual bool IsRoutingOptionsGood(Segment const & /* segment */);
  virtual RoutingOptions GetRoutingOptions(Segment const & /* segment */);
  virtual void SetRoutingOptions(RoutingOptions /* routingOptions */);

  virtual void SetAStarParents(bool forward, Parents<Segment> & parents);
  virtual void SetAStarParents(bool forward, Parents<JointSegment> & parents);
  virtual void DropAStarParents();

  virtual bool AreWavesConnectible(Parents<Segment> & forwardParents, Segment const & commonVertex,
                                   Parents<Segment> & backwardParents);

  using FakeConverterT = std::function<void(JointSegment &)>;
  virtual bool AreWavesConnectible(Parents<JointSegment> & forwardParents, JointSegment const & commonVertex,
                                   Parents<JointSegment> & backwardParents,
                                   FakeConverterT const & fakeFeatureConverter);

  /// \returns transit-specific information for segment. For nontransit segments returns nullptr.
  virtual std::unique_ptr<TransitInfo> GetTransitInfo(Segment const & segment);

  virtual std::vector<RouteSegment::SpeedCamera> GetSpeedCamInfo(Segment const & segment);
  virtual SpeedInUnits GetSpeedLimit(Segment const & segment);

  virtual IndexGraph & GetIndexGraph(NumMwmId numMwmId) = 0;
  virtual CrossMwmGraph & GetCrossMwmGraph();
  virtual void GetTwinsInner(Segment const & segment, bool isOutgoing, std::vector<Segment> & twins) = 0;

  virtual RouteWeight GetCrossBorderPenalty(NumMwmId mwmId1, NumMwmId mwmId2);

protected:
  void GetTwins(Segment const & segment, bool isOutgoing, bool useRoutingOptions, SegmentEdgeListT & edges);

  bool m_isRegionsGraphMode = false;
};

std::string DebugPrint(WorldGraphMode mode);
}  // namespace routing
