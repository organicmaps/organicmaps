#include "routing/cross_mwm_ramp.hpp"

namespace
{
uint32_t constexpr kFakeId = std::numeric_limits<uint32_t>::max();
}  // namespace

namespace routing
{
// static
CrossMwmRamp::Weight constexpr CrossMwmRamp::kNoRoute;

void CrossMwmRamp::AddTransition(uint32_t featureId, uint32_t segmentIdx, bool oneWay,
                                 bool forwardIsEnter, m2::PointD const & backPoint,
                                 m2::PointD const & frontPoint)
{
  Transition transition(kFakeId, kFakeId, oneWay, forwardIsEnter, backPoint, frontPoint);

  if (forwardIsEnter)
  {
    transition.m_enterIdx = base::asserted_cast<uint32_t>(m_enters.size());
    m_enters.emplace_back(m_mwmId, featureId, segmentIdx, true);
  }
  else
  {
    transition.m_exitIdx = base::asserted_cast<uint32_t>(m_exits.size());
    m_exits.emplace_back(m_mwmId, featureId, segmentIdx, true);
  }

  if (!oneWay)
  {
    if (forwardIsEnter)
    {
      transition.m_exitIdx = base::asserted_cast<uint32_t>(m_exits.size());
      m_exits.emplace_back(m_mwmId, featureId, segmentIdx, false);
    }
    else
    {
      transition.m_enterIdx = base::asserted_cast<uint32_t>(m_enters.size());
      m_enters.emplace_back(m_mwmId, featureId, segmentIdx, false);
    }
  }

  m_transitions[Key(featureId, segmentIdx)] = transition;
}

bool CrossMwmRamp::IsTransition(Segment const & segment, bool isOutgoing) const
{
  auto it = m_transitions.find(Key(segment.GetFeatureId(), segment.GetSegmentIdx()));
  if (it == m_transitions.cend())
    return false;

  Transition const & transition = it->second;
  if (transition.m_oneWay && !segment.IsForward())
    return false;

  return (segment.IsForward() == transition.m_forwardIsEnter) == isOutgoing;
}

m2::PointD const & CrossMwmRamp::GetPoint(Segment const & segment, bool front) const
{
  Transition const & transition = GetTransition(segment);
  return segment.IsForward() == front ? transition.m_frontPoint : transition.m_backPoint;
}

void CrossMwmRamp::GetEdgeList(Segment const & segment, bool isOutgoing,
                               std::vector<SegmentEdge> & edges) const
{
  Transition const & transition = GetTransition(segment);
  if (isOutgoing)
  {
    ASSERT_NOT_EQUAL(transition.m_enterIdx, kFakeId, ());
    for (size_t exitIdx = 0; exitIdx < m_exits.size(); ++exitIdx)
    {
      Weight const weight = GetWeight(base::asserted_cast<size_t>(transition.m_enterIdx), exitIdx);
      AddEdge(m_exits[exitIdx], weight, edges);
    }
  }
  else
  {
    ASSERT_NOT_EQUAL(transition.m_exitIdx, kFakeId, ());
    for (size_t enterIdx = 0; enterIdx < m_enters.size(); ++enterIdx)
    {
      Weight const weight = GetWeight(enterIdx, base::asserted_cast<size_t>(transition.m_exitIdx));
      AddEdge(m_enters[enterIdx], weight, edges);
    }
  }
}

void CrossMwmRamp::AddEdge(Segment const & segment, Weight weight,
                           std::vector<SegmentEdge> & edges) const
{
  if (weight != kNoRoute)
    edges.emplace_back(segment, static_cast<double>(weight));
}

CrossMwmRamp::Transition const & CrossMwmRamp::GetTransition(Segment const & segment) const
{
  auto it = m_transitions.find(Key(segment.GetFeatureId(), segment.GetSegmentIdx()));
  CHECK(it != m_transitions.cend(), ("Not transition segment:", segment));
  return it->second;
}

CrossMwmRamp::Weight CrossMwmRamp::GetWeight(size_t enterIdx, size_t exitIdx) const
{
  size_t const i = enterIdx * m_exits.size() + exitIdx;
  ASSERT_LESS(i, m_weights.size(), ());
  return m_weights[i];
}
}  // namespace routing
