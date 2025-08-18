#pragma once

#include "routing/latlon_with_altitude.hpp"
#include "routing/world_graph.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/latlon.hpp"

#include "base/assert.hpp"

#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace routing
{
// This is dummy class inherited from WorldGraph. Some of its overridden methods should never be
// called. This class is used in RegionsRouter as a lightweight replacement of
// SingleVehicleWorldGraph.
class DummyWorldGraph final : public WorldGraph
{
public:
  using WorldGraph::GetEdgeList;

  void GetEdgeList(astar::VertexData<Segment, RouteWeight> const & vertexData, bool isOutgoing, bool useRoutingOptions,
                   bool useAccessConditional, SegmentEdgeListT & edges) override
  {
    UNREACHABLE();
  }

  void GetEdgeList(astar::VertexData<JointSegment, RouteWeight> const & vertexData, Segment const & segment,
                   bool isOutgoing, bool useAccessConditional, JointEdgeListT & edges,
                   WeightListT & parentWeights) override
  {
    UNREACHABLE();
  }

  bool CheckLength(RouteWeight const & weight, double startToFinishDistanceM) const override { return true; }

  LatLonWithAltitude const & GetJunction(Segment const & segment, bool front) override { UNREACHABLE(); }

  ms::LatLon const & GetPoint(Segment const & segment, bool front) override { UNREACHABLE(); }

  bool IsOneWay(NumMwmId mwmId, uint32_t featureId) override { UNREACHABLE(); }

  bool IsPassThroughAllowed(NumMwmId mwmId, uint32_t featureId) override { UNREACHABLE(); }

  void ClearCachedGraphs() override { UNREACHABLE(); }

  void SetMode(WorldGraphMode mode) override {}

  WorldGraphMode GetMode() const override { return WorldGraphMode::NoLeaps; }

  RouteWeight HeuristicCostEstimate(ms::LatLon const & from, ms::LatLon const & to) override
  {
    return RouteWeight(ms::DistanceOnEarth(from, to));
  }

  RouteWeight CalcSegmentWeight(Segment const & segment, EdgeEstimator::Purpose purpose) override { UNREACHABLE(); }

  RouteWeight CalcLeapWeight(ms::LatLon const & from, ms::LatLon const & to, NumMwmId mwmId) const override
  {
    UNREACHABLE();
  }

  RouteWeight CalcOffroadWeight(ms::LatLon const & from, ms::LatLon const & to,
                                EdgeEstimator::Purpose purpose) const override
  {
    return RouteWeight(ms::DistanceOnEarth(from, to));
  }

  double CalculateETA(Segment const & from, Segment const & to) override { UNREACHABLE(); }

  double CalculateETAWithoutPenalty(Segment const & segment) override { UNREACHABLE(); }

  IndexGraph & GetIndexGraph(NumMwmId numMwmId) override { UNREACHABLE(); }

  void GetTwinsInner(Segment const & segment, bool isOutgoing, std::vector<Segment> & twins) override
  {
    CHECK(false, ());
  }
};
}  // namespace routing
