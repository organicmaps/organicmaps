#pragma once

#include "routing/edge_estimator.hpp"
#include "routing/fake_ending.hpp"
#include "routing/fake_graph.hpp"
#include "routing/fake_vertex.hpp"
#include "routing/latlon_with_altitude.hpp"
#include "routing/route_weight.hpp"
#include "routing/segment.hpp"

#include "transit/experimental/transit_data.hpp"
#include "transit/experimental/transit_types_experimental.hpp"
#include "transit/transit_graph_data.hpp"
#include "transit/transit_schedule.hpp"
#include "transit/transit_types.hpp"
#include "transit/transit_version.hpp"

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
  // Fake endings for gates (in subway transit version) or for gates and stops (in public transport
  // version.
  using Endings = std::map<transit::OsmId, FakeEnding>;

  static bool IsTransitFeature(uint32_t featureId);
  static bool IsTransitSegment(Segment const & segment);

  TransitGraph(::transit::TransitVersion transitVersion, NumMwmId numMwmId, std::shared_ptr<EdgeEstimator> estimator);

  ::transit::TransitVersion GetTransitVersion() const;

  LatLonWithAltitude const & GetJunction(Segment const & segment, bool front) const;
  RouteWeight CalcSegmentWeight(Segment const & segment, EdgeEstimator::Purpose purpose) const;
  RouteWeight GetTransferPenalty(Segment const & from, Segment const & to) const;

  using EdgeListT = SmallList<SegmentEdge>;
  void GetTransitEdges(Segment const & segment, bool isOutgoing, EdgeListT & edges) const;
  std::set<Segment> const & GetFake(Segment const & real) const;
  bool FindReal(Segment const & fake, Segment & real) const;

  void Fill(::transit::experimental::TransitData const & transitData, Endings const & stopEndings,
            Endings const & gateEndings);
  void Fill(transit::GraphData const & transitData, Endings const & gateEndings);

  bool IsGate(Segment const & segment) const;
  bool IsEdge(Segment const & segment) const;
  transit::Gate const & GetGate(Segment const & segment) const;
  ::transit::experimental::Gate const & GetGatePT(Segment const & segment) const;
  transit::Edge const & GetEdge(Segment const & segment) const;
  ::transit::experimental::Edge const & GetEdgePT(Segment const & segment) const;

private:
  using StopToSegmentsMap = std::map<transit::StopId, std::set<Segment>>;

  Segment GetTransitSegment(uint32_t segmentIdx) const;
  Segment GetNewTransitSegment() const;

  // Adds gate to fake graph. Also adds gate to temporary stopToBack, stopToFront maps used while
  // TransitGraph::Fill.
  void AddGate(transit::Gate const & gate, FakeEnding const & ending,
               std::map<transit::StopId, LatLonWithAltitude> const & stopCoords, bool isEnter,
               StopToSegmentsMap & stopToBack, StopToSegmentsMap & stopToFront);

  void AddGate(::transit::experimental::Gate const & gate, FakeEnding const & ending,
               std::map<transit::StopId, LatLonWithAltitude> const & stopCoords, bool isEnter,
               StopToSegmentsMap & stopToBack, StopToSegmentsMap & stopToFront);

  void AddStop(::transit::experimental::Stop const & stop, FakeEnding const & ending,
               std::map<transit::StopId, LatLonWithAltitude> const & stopCoords, StopToSegmentsMap & stopToBack,
               StopToSegmentsMap & stopToFront);

  // Adds transit edge to fake graph, returns corresponding transit segment. Also adds gate to
  // temporary stopToBack, stopToFront maps used while TransitGraph::Fill.
  Segment AddEdge(transit::Edge const & edge, std::map<transit::StopId, LatLonWithAltitude> const & stopCoords,
                  StopToSegmentsMap & stopToBack, StopToSegmentsMap & stopToFront);

  Segment AddEdge(::transit::experimental::Edge const & edge,
                  std::map<transit::StopId, LatLonWithAltitude> const & stopCoords, StopToSegmentsMap & stopToBack,
                  StopToSegmentsMap & stopToFront);

  // Adds connections to fake graph.
  void AddConnections(StopToSegmentsMap const & connections, StopToSegmentsMap const & stopToBack,
                      StopToSegmentsMap const & stopToFront, bool isOutgoing);

  ::transit::TransitVersion const m_transitVersion;
  NumMwmId const m_mwmId = kFakeNumMwmId;
  std::shared_ptr<EdgeEstimator> m_estimator;
  FakeGraph m_fake;

  // Fields for working with OnlySubway version of transit.
  std::map<Segment, transit::Edge> m_segmentToEdgeSubway;
  std::map<Segment, transit::Gate> m_segmentToGateSubway;
  std::map<transit::LineId, double> m_transferPenaltiesSubway;

  // Fields for working with Public transport version of transit.
  std::map<Segment, ::transit::experimental::Edge> m_segmentToEdgePT;
  std::map<Segment, ::transit::experimental::Gate> m_segmentToGatePT;
  std::map<Segment, ::transit::experimental::Stop> m_segmentToStopPT;
  std::map<::transit::TransitId, ::transit::Schedule> m_transferPenaltiesPT;
};

void MakeGateEndings(std::vector<transit::Gate> const & gates, NumMwmId mwmId, IndexGraph & indexGraph,
                     TransitGraph::Endings & gateEndings);

void MakeGateEndings(std::vector<::transit::experimental::Gate> const & gates, NumMwmId mwmId, IndexGraph & indexGraph,
                     TransitGraph::Endings & gateEndings);

void MakeStopEndings(std::vector<::transit::experimental::Stop> const & stops, NumMwmId mwmId, IndexGraph & indexGraph,
                     TransitGraph::Endings & gateEndings);
}  // namespace routing
