#pragma once

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
  static bool IsTransitFeature(uint32_t featureId) { return featureId == kTransitFeatureId; }

  static bool IsTransitSegment(Segment const & segment)
  {
    return IsTransitFeature(segment.GetFeatureId());
  }

  Junction const & GetJunction(Segment const & segment, bool front) const
  {
    CHECK(IsTransitSegment(segment), ("Nontransit segment passed to TransitGraph."));
    auto const & vertex = m_fake.GetVertex(segment);
    return front ? vertex.GetJunctionTo() : vertex.GetJunctionFrom();
  }

  RouteWeight CalcSegmentWeight(Segment const & segment) const
  {
    CHECK(IsTransitSegment(segment), ("Nontransit segment passed to TransitGraph."));
    if (IsGate(segment))
      return RouteWeight(GetGate(segment).GetWeight(), 0 /* nontransitCross */);

    CHECK(IsEdge(segment), ("Unknown transit segment."));
    return RouteWeight(GetEdge(segment).GetWeight(), 0 /* nontransitCross */);
  }

  void GetTransitEdges(Segment const & segment, bool isOutgoing, std::vector<SegmentEdge> & edges) const
  {
    CHECK(IsTransitSegment(segment), ("Nontransit segment passed to TransitGraph."));
    for (auto const & s : m_fake.GetEdges(segment, isOutgoing))
      edges.emplace_back(s, CalcSegmentWeight(isOutgoing ? s : segment));
  }

  std::set<Segment> const & GetFake(Segment const & real) const { return m_fake.GetFake(real); }

  bool FindReal(Segment const & fake, Segment & real) const { return m_fake.FindReal(fake, real); }

private:
  bool IsGate(Segment const & segment) const { return m_segmentToGate.count(segment) > 0; }
  bool IsEdge(Segment const & segment) const { return m_segmentToEdge.count(segment) > 0; }

  transit::Edge const & GetEdge(Segment const & segment) const
  {
    auto const it = m_segmentToEdge.find(segment);
    CHECK(it != m_segmentToEdge.cend(), ("Unknown transit segment."));
    return it->second;
  }

  transit::Gate const & GetGate(Segment const & segment) const
  {
    auto const it = m_segmentToGate.find(segment);
    CHECK(it != m_segmentToGate.cend(), ("Unknown transit segment."));
    return it->second;
  }

  static uint32_t constexpr kTransitFeatureId = FakeFeatureIds::kTransitGraphId;
  FakeGraph<Segment, FakeVertex, Segment> m_fake;
  std::map<Segment, transit::Edge> m_segmentToEdge;
  std::map<Segment, transit::Gate> m_segmentToGate;
};
}  // namespace routing
