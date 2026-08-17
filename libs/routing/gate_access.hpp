#pragma once

#include "routing/fake_ending.hpp"
#include "routing/segment.hpp"

#include <vector>

namespace routing
{
// A transit gate's pedestrian access point. |m_projection| is the gate's road projection.
// |m_entrySegment| is the transit fake segment to hop onto to board (entrance) or to leave after
// alighting (exit). Used to add nearby gates as start/finish snapping candidates.
struct GateAccess
{
  GateAccess(Projection const & projection, Segment const & entrySegment, bool isEnter)
    : m_projection(projection)
    , m_entrySegment(entrySegment)
    , m_isEnter(isEnter)
  {}

  Projection m_projection;
  Segment m_entrySegment;
  bool m_isEnter;
};

using GateAccessesT = std::vector<GateAccess>;
}  // namespace routing
