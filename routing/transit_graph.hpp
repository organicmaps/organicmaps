#pragma once

#include "routing/edge_estimator.hpp"
#include "routing/fake_ending.hpp"
#include "routing/fake_feature_ids.hpp"
#include "routing/fake_graph.hpp"
#include "routing/fake_vertex.hpp"
#include "routing/road_graph.hpp"
#include "routing/route_weight.hpp"
#include "routing/segment.hpp"

#include "routing_common/transit_types.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <vector>

namespace routing
{
class TransitGraph final
{
public:
  static bool IsTransitFeature(uint32_t featureId);
  static bool IsTransitSegment(Segment const & segment);

  TransitGraph(NumMwmId numMwmId, std::shared_ptr<EdgeEstimator> estimator);

  Junction const & GetJunction(Segment const & segment, bool front) const;
  RouteWeight CalcSegmentWeight(Segment const & segment) const;
  RouteWeight GetTransferPenalty(Segment const & from, Segment const & to) const;
  void GetTransitEdges(Segment const & segment, bool isOutgoing,
                       std::vector<SegmentEdge> & edges) const;
  std::set<Segment> const & GetFake(Segment const & real) const;
  bool FindReal(Segment const & fake, Segment & real) const;

  void Fill(std::vector<transit::Stop> const & stops, std::vector<transit::Edge> const & edges,
            std::vector<transit::Line> const & lines, std::vector<transit::Gate> const & gates,
            std::map<transit::OsmId, FakeEnding> const & gateEndings);

  bool IsGate(Segment const & segment) const;
  bool IsEdge(Segment const & segment) const;
  transit::Gate const & GetGate(Segment const & segment) const;
  transit::Edge const & GetEdge(Segment const & segment) const;

private:
  using StopToSegmentsMap = std::map<transit::StopId, std::set<Segment>>;

  Segment GetTransitSegment(uint32_t segmentIdx) const;
  Segment GetNewTransitSegment() const;

  // Adds gate to fake graph. Also adds gate to temporary stopToBack, stopToFront maps used while
  // TransitGraph::Fill.
  void AddGate(transit::Gate const & gate, FakeEnding const & ending,
               std::map<transit::StopId, Junction> const & stopCoords, bool isEnter,
               StopToSegmentsMap & stopToBack, StopToSegmentsMap & stopToFront);
  // Adds transit edge to fake graph, returns corresponding transit segment. Also adds gate to
  // temporary stopToBack, stopToFront maps used while TransitGraph::Fill.
  Segment AddEdge(transit::Edge const & edge,
                  std::map<transit::StopId, Junction> const & stopCoords,
                  StopToSegmentsMap & stopToBack, StopToSegmentsMap & stopToFront);
  // Adds connections to fake graph.
  void AddConnections(StopToSegmentsMap const & connections, StopToSegmentsMap const & stopToBack,
                      StopToSegmentsMap const & stopToFront, bool isOutgoing);

  static uint32_t constexpr kTransitFeatureId = FakeFeatureIds::kTransitGraphId;
  NumMwmId const m_mwmId = kFakeNumMwmId;
  std::shared_ptr<EdgeEstimator> m_estimator;
  FakeGraph<Segment, FakeVertex, Segment> m_fake;
  std::map<Segment, transit::Edge> m_segmentToEdge;
  std::map<Segment, transit::Gate> m_segmentToGate;
  std::map<transit::LineId, double> m_transferPenalties;
};
}  // namespace routing
