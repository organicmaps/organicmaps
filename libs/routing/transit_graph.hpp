#pragma once

#include "routing/edge_estimator.hpp"
#include "routing/fake_ending.hpp"
#include "routing/fake_graph.hpp"
#include "routing/fake_vertex.hpp"
#include "routing/gate_access.hpp"
#include "routing/latlon_with_altitude.hpp"
#include "routing/route_weight.hpp"
#include "routing/segment.hpp"

#include "transit/transit_graph_data.hpp"
#include "transit/transit_types.hpp"

#include "routing_common/num_mwm_id.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <vector>

namespace routing
{
class IndexGraph;

class TransitGraph final
{
public:
  // Fake endings for transit gates.
  using Endings = std::map<transit::OsmId, FakeEnding>;

  struct GateInfo
  {
    transit::Gate m_gate;
    bool m_isEntrance;
  };

  static bool IsTransitFeature(uint32_t featureId);
  static bool IsTransitSegment(Segment const & segment);

  TransitGraph(NumMwmId numMwmId, std::shared_ptr<EdgeEstimator> estimator);

  LatLonWithAltitude const & GetJunction(Segment const & segment, bool front) const;
  // Gate, transfer and projection segments are walked. Only search weights include the walking bias.
  RouteWeight CalcSegmentWeight(Segment const & segment, EdgeEstimator::Purpose purpose) const;
  transit::LineId GetRideLineId(Segment const & segment) const;
  RouteWeight GetTransferPenalty(transit::LineId fromLineId, Segment const & to) const;

  // Fake (transit graph) neighbours of a transit |segment|.
  std::set<Segment> const & GetFakeEdges(Segment const & segment, bool isOutgoing) const
  {
    return m_fake.GetEdges(segment, isOutgoing);
  }
  std::set<Segment> const & GetFake(Segment const & real) const;
  bool FindReal(Segment const & fake, Segment & real) const;

  // Fills |out| with gate accesses (see GateAccess) within |radiusM| of |point| (mercator) matching
  // |isEnter|.
  void GetGatesNear(m2::PointD const & point, double radiusM, bool isEnter, GateAccessesT & out) const;

  void Fill(transit::GraphData const & transitData, Endings const & gateEndings);

  // Return nullptr if |segment| is not a gate/edge.
  GateInfo const * FindGate(Segment const & segment) const;
  transit::Edge const * FindEdge(Segment const & segment) const;

private:
  using StopToSegmentsMap = std::map<transit::StopId, std::set<Segment>>;

  Segment GetTransitSegment(uint32_t featureId) const;
  Segment GetNewTransitSegment() const;

  // Adds gate to fake graph. Also adds gate to temporary stopToBack, stopToFront maps used while
  // TransitGraph::Fill.
  void AddGate(transit::Gate const & gate, FakeEnding const & ending,
               std::map<transit::StopId, LatLonWithAltitude> const & stopCoords, bool isEnter,
               StopToSegmentsMap & stopToBack, StopToSegmentsMap & stopToFront);

  // Adds transit edge to fake graph, returns corresponding transit segment. Also adds the edge to
  // temporary stopToBack, stopToFront maps used while TransitGraph::Fill.
  Segment AddEdge(transit::Edge const & edge, std::map<transit::StopId, LatLonWithAltitude> const & stopCoords,
                  StopToSegmentsMap & stopToBack, StopToSegmentsMap & stopToFront);

  // Adds connections to fake graph.
  void AddConnections(StopToSegmentsMap const & connections, StopToSegmentsMap const & stopToBack,
                      StopToSegmentsMap const & stopToFront, bool isOutgoing);

  NumMwmId const m_mwmId = kFakeNumMwmId;
  std::shared_ptr<EdgeEstimator> m_estimator;
  FakeGraph m_fake;

  // Pedestrian access points of all gates, recorded while filling the graph (see AddGate).
  GateAccessesT m_gateAccesses;

  std::map<Segment, transit::Edge> m_segmentToEdgeSubway;
  std::map<Segment, GateInfo> m_segmentToGateSubway;
  std::map<transit::LineId, double> m_transferPenaltiesSubway;
};

void MakeGateEndings(std::vector<transit::Gate> const & gates, NumMwmId mwmId, IndexGraph & indexGraph,
                     TransitGraph::Endings & gateEndings);

}  // namespace routing
