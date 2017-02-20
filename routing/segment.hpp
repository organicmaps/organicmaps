#pragma once

#include "routing/num_mwm_id.hpp"
#include "routing/road_point.hpp"

#include "std/cstdint.hpp"
#include "std/sstream.hpp"
#include "std/string.hpp"

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
    : m_featureId(featureId), m_segmentIdx(segmentIdx), m_mwmId(mwmId), m_forward(forward)
  {
  }

  NumMwmId GetMwmId() const { return m_mwmId; }
  uint32_t GetFeatureId() const { return m_featureId; }
  uint32_t GetSegmentIdx() const { return m_segmentIdx; }
  bool IsForward() const { return m_forward; }

  uint32_t GetPointId(bool front) const
  {
    return m_forward == front ? m_segmentIdx + 1 : m_segmentIdx;
  }

  RoadPoint GetRoadPoint(bool front) const { return RoadPoint(m_featureId, GetPointId(front)); }

  bool operator<(Segment const & seg) const
  {
    if (m_featureId != seg.m_featureId)
      return m_featureId < seg.m_featureId;

    if (m_segmentIdx != seg.m_segmentIdx)
      return m_segmentIdx < seg.m_segmentIdx;

    if (m_mwmId != seg.m_mwmId)
      return m_mwmId < seg.m_mwmId;

    return m_forward < seg.m_forward;
  }

  bool operator==(Segment const & seg) const
  {
    return m_featureId == seg.m_featureId && m_segmentIdx == seg.m_segmentIdx &&
           m_mwmId == seg.m_mwmId && m_forward == seg.m_forward;
  }

  bool operator!=(Segment const & seg) const { return !(*this == seg); }

private:
  uint32_t m_featureId = 0;
  uint32_t m_segmentIdx = 0;
  NumMwmId m_mwmId = kFakeNumMwmId;
  bool m_forward = false;
};

class SegmentEdge final
{
public:
  SegmentEdge(Segment const & target, double weight) : m_target(target), m_weight(weight) {}
  Segment const & GetTarget() const { return m_target; }
  double GetWeight() const { return m_weight; }

private:
  // Target is vertex going to for outgoing edges, vertex going from for ingoing edges.
  Segment m_target;
  double m_weight;
};

inline string DebugPrint(Segment const & segment)
{
  ostringstream out;
  out << "Segment(" << segment.GetMwmId() << ", " << segment.GetFeatureId() << ", "
      << segment.GetSegmentIdx() << ", " << segment.IsForward() << ")";
  return out.str();
}
}  // namespace routing
