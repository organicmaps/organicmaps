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
#include <set>
#include <vector>

namespace routing
{
class TransitGraph final
{
public:
  static bool IsTransitFeature(uint32_t featureId);
  static bool IsTransitSegment(Segment const & segment);

  Junction const & GetJunction(Segment const & segment, bool front) const;
  RouteWeight CalcSegmentWeight(Segment const & segment, EdgeEstimator const & estimator) const;
  RouteWeight GetTransferPenalty(Segment const & from, Segment const & to) const;
  void GetTransitEdges(Segment const & segment, bool isOutgoing, std::vector<SegmentEdge> & edges,
                       EdgeEstimator const & estimator) const;
  std::set<Segment> const & GetFake(Segment const & real) const;
  bool FindReal(Segment const & fake, Segment & real) const;

  // Fills transit info based on data from transit section.
  // Uses |indexGraph| to get road geometry: only modifications of |indexGraph| are
  // modifications of geometry cache.
  // TODO (t.yan) get rid of indexGraph and estimator
  void Fill(std::vector<transit::Stop> const & stops, std::vector<transit::Gate> const & gates,
            std::vector<transit::Edge> const & edges, std::vector<transit::Line> const & lines,
            EdgeEstimator const & estimator, NumMwmId numMwmId, IndexGraph & indexGraph);

  bool IsGate(Segment const & segment) const;
  bool IsEdge(Segment const & segment) const;
  transit::Gate const & GetGate(Segment const & segment) const;
  transit::Edge const & GetEdge(Segment const & segment) const;

private:
  Segment GetTransitSegment(uint32_t segmentIdx) const;
  Segment GetNewTransitSegment() const;

  void AddGate(transit::Gate const & gate, FakeEnding const & ending,
               std::map<transit::StopId, Junction> const & stopCoords, bool isEnter);
  // Adds transit edge to fake graph, returns corresponding transit segment.
  Segment AddEdge(transit::Edge const & edge,
                  std::map<transit::StopId, Junction> const & stopCoords);
  void AddConnections(std::map<transit::StopId, std::set<Segment>> const & connections,
                      bool isOutgoing);

  static uint32_t constexpr kTransitFeatureId = FakeFeatureIds::kTransitGraphId;
  NumMwmId m_mwmId = kFakeNumMwmId;
  FakeGraph<Segment, FakeVertex, Segment> m_fake;
  std::map<Segment, transit::Edge> m_segmentToEdge;
  std::map<Segment, transit::Gate> m_segmentToGate;
  std::map<transit::LineId, double> m_transferPenalties;
  // TODO (@t.yan) move m_edgeToSegment, m_stopToBack, m_stopToFront to Fill
  std::map<transit::StopId, std::set<Segment>> m_stopToBack;
  std::map<transit::StopId, std::set<Segment>> m_stopToFront;
};
}  // namespace routing
