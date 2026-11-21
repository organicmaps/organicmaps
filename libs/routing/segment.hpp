#pragma once

#include "routing/road_point.hpp"
#include "routing/route_weight.hpp"

#include "routing_common/num_mwm_id.hpp"

#include <string>

namespace routing
{
// This is directed road segment used as vertex in index graph.
//
// You can imagine the segment as a material arrow.
// Head of each arrow is connected to fletchings of next arrows with invisible links:
// these are the edges of the graph.
//
// Position of the segment is a position of the arrowhead: GetPointId(true).
// This position is used in heuristic and edges weight calculations.
class Segment final
{
public:
  Segment() = default;

  constexpr Segment(NumMwmId mwmId, uint32_t featureId, uint32_t segmentIdx, bool forward)
    : m_featureId(featureId)
    , m_segmentIdx(segmentIdx)
    , m_mwmId(mwmId)
    , m_forward(forward)
  {}

  Segment const & GetSegment(bool /* start */) const { return *this; }
  NumMwmId GetMwmId() const { return m_mwmId; }
  uint32_t GetFeatureId() const { return m_featureId; }
  uint32_t GetSegmentIdx() const { return m_segmentIdx; }
  bool IsForward() const { return m_forward; }

  uint32_t GetPointId(bool front) const;

  uint32_t GetMinPointId() const { return m_segmentIdx; }
  uint32_t GetMaxPointId() const { return m_segmentIdx + 1; }

  RoadPoint GetRoadPoint(bool front) const { return RoadPoint(m_featureId, GetPointId(front)); }

  bool operator<(Segment const & seg) const;
  bool operator==(Segment const & seg) const;
  bool operator!=(Segment const & seg) const { return !(*this == seg); }

  bool IsInverse(Segment const & seg) const;
  Segment GetReversed() const { return {m_mwmId, m_featureId, m_segmentIdx, !m_forward}; }

  /// @todo Logically, these functions should be equal, but keep existing logic,
  /// and investigate possible enhancements in future.
  /// @{
  bool IsFakeCreated() const;
  bool IsRealSegment() const;
  /// @}

  void Next(bool forward) { forward ? ++m_segmentIdx : --m_segmentIdx; }

private:
  uint32_t m_featureId = 0;
  uint32_t m_segmentIdx = 0;
  NumMwmId m_mwmId = kFakeNumMwmId;
  bool m_forward = false;
};

// Keep the charged segment cost so a partial edge can retain the same speed and crossing penalties.
class SegmentEdgeWeight
{
public:
  SegmentEdgeWeight() = default;
  // This overload treats the total as segment cost. Keep separate penalties for partial edges.
  explicit SegmentEdgeWeight(RouteWeight const & weight) : m_total(weight), m_segmentWeight(weight.GetWeight()) {}
  SegmentEdgeWeight(double segmentWeight, RouteWeight const & penalties)
    : m_total(RouteWeight(segmentWeight) + penalties)
    , m_segmentWeight(segmentWeight)
  {}

  RouteWeight const & GetTotal() const { return m_total; }

  SegmentEdgeWeight ForSegmentFraction(double fraction) const
  {
    auto partial = *this;
    partial.m_segmentWeight *= fraction;
    partial.m_total += RouteWeight(partial.m_segmentWeight - m_segmentWeight);
    return partial;
  }

private:
  RouteWeight m_total;
  double m_segmentWeight = 0.0;
};

class SegmentEdge final
{
public:
  SegmentEdge() = default;
  SegmentEdge(Segment const & target, RouteWeight const & weight) : SegmentEdge(target, SegmentEdgeWeight(weight)) {}
  SegmentEdge(Segment const & target, SegmentEdgeWeight const & weight) : m_target(target), m_weight(weight) {}

  Segment const & GetTarget() const { return m_target; }
  RouteWeight const & GetWeight() const { return m_weight.GetTotal(); }
  SegmentEdgeWeight const & GetWeightComponents() const { return m_weight; }
  /// @note Used in tests only!
  bool operator==(SegmentEdge const & edge) const;

private:
  // Target is vertex going to for outgoing edges, vertex going from for ingoing edges.
  Segment m_target;
  SegmentEdgeWeight m_weight;
};

std::string DebugPrint(Segment const & segment);
std::string DebugPrint(SegmentEdge const & edge);
}  // namespace routing

namespace std
{
template <>
struct hash<routing::Segment>
{
  size_t operator()(routing::Segment const & segment) const;
};
}  // namespace std
