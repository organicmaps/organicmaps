#pragma once

#include "routing/road_point.hpp"
#include "routing/route_weight.hpp"
#include "routing/segment.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <limits>
#include <string>

namespace routing
{
class JointSegment
{
public:
  JointSegment() = default;
  JointSegment(Segment const & from, Segment const & to);

  uint32_t & GetFeatureId() { return m_featureId; }
  uint32_t GetFeatureId() const { return m_featureId; }
  NumMwmId GetMwmId() const { return m_numMwmId; }
  NumMwmId & GetMwmId() { return m_numMwmId; }
  uint32_t GetStartSegmentId() const { return m_startSegmentId; }
  uint32_t GetEndSegmentId() const { return m_endSegmentId; }
  uint32_t GetSegmentId(bool start) const { return start ? m_startSegmentId : m_endSegmentId; }
  bool IsForward() const { return m_forward; }

  void ToFake(uint32_t fakeId);
  bool IsFake() const;

  Segment GetSegment(bool start) const
  {
    return {m_numMwmId, m_featureId, start ? m_startSegmentId : m_endSegmentId, m_forward};
  }

  bool operator<(JointSegment const & rhs) const;
  bool operator==(JointSegment const & rhs) const;

private:
  static uint32_t constexpr kInvalidId = std::numeric_limits<uint32_t>::max();
  uint32_t m_featureId = kInvalidId;
  uint32_t m_startSegmentId = kInvalidId;
  uint32_t m_endSegmentId = kInvalidId;
  NumMwmId m_numMwmId = kFakeNumMwmId;
  bool m_forward = false;
};

class JointEdge
{
public:
  JointEdge(JointSegment const & target, RouteWeight const & weight)
    : m_target(target), m_weight(weight) {}

  JointSegment const & GetTarget() const { return m_target; }
  JointSegment & GetTarget() { return m_target; }
  RouteWeight & GetWeight() { return m_weight; }
  RouteWeight const & GetWeight() const { return m_weight; }

private:
  JointSegment m_target;
  RouteWeight m_weight;
};

std::string DebugPrint(JointSegment const & jointSegment);
}  // namespace routing
