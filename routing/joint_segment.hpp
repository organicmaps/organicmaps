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
  static auto constexpr kInvalidSegmentId = std::numeric_limits<uint32_t>::max();
  static auto constexpr kInvalidFeatureIdId = std::numeric_limits<uint32_t>::max();

  JointSegment() = default;
  JointSegment(Segment const & from, Segment const & to);

  void SetFeatureId(uint32_t id) { m_featureId = id; }
  uint32_t GetFeatureId() const { return m_featureId; }
  NumMwmId GetMwmId() const { return m_numMwmId; }
  void SetMwmId(NumMwmId id) { m_numMwmId = id; }
  uint32_t GetStartSegmentId() const { return m_startSegmentId; }
  uint32_t GetEndSegmentId() const { return m_endSegmentId; }
  uint32_t GetSegmentId(bool start) const { return start ? m_startSegmentId : m_endSegmentId; }
  bool IsForward() const { return m_forward; }

  void ToFake(uint32_t fakeId);
  bool IsFake() const;
  bool IsRealSegment() const { return !IsFake(); }

  Segment GetSegment(bool start) const;

  bool operator<(JointSegment const & rhs) const;
  bool operator==(JointSegment const & rhs) const;
  bool operator!=(JointSegment const & rhs) const;

private:
  uint32_t m_featureId = kInvalidFeatureIdId;
  uint32_t m_startSegmentId = kInvalidSegmentId;
  uint32_t m_endSegmentId = kInvalidSegmentId;
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

namespace std
{
template <>
struct hash<routing::JointSegment>
{
  size_t operator()(routing::JointSegment const & jointSegment) const;
};
}  // namespace std
