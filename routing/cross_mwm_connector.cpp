#include "routing/cross_mwm_connector.hpp"

namespace
{
uint32_t constexpr kFakeId = std::numeric_limits<uint32_t>::max();
}  // namespace

namespace routing
{
// static
CrossMwmConnector::Weight constexpr CrossMwmConnector::kNoRoute;

void CrossMwmConnector::AddTransition(uint32_t featureId, uint32_t segmentIdx, bool oneWay,
                                      bool forwardIsEnter, m2::PointD const & backPoint,
                                      m2::PointD const & frontPoint)
{
  Transition transition(kFakeId, kFakeId, oneWay, forwardIsEnter, backPoint, frontPoint);

  if (forwardIsEnter)
  {
    transition.m_enterIdx = base::asserted_cast<uint32_t>(m_enters.size());
    m_enters.emplace_back(m_mwmId, featureId, segmentIdx, true /* forward */);
  }
  else
  {
    transition.m_exitIdx = base::asserted_cast<uint32_t>(m_exits.size());
    m_exits.emplace_back(m_mwmId, featureId, segmentIdx, true /* forward */);
  }

  if (!oneWay)
  {
    if (forwardIsEnter)
    {
      transition.m_exitIdx = base::asserted_cast<uint32_t>(m_exits.size());
      m_exits.emplace_back(m_mwmId, featureId, segmentIdx, false /* forward */);
    }
    else
    {
      transition.m_enterIdx = base::asserted_cast<uint32_t>(m_enters.size());
      m_enters.emplace_back(m_mwmId, featureId, segmentIdx, false /* forward */);
    }
  }

  m_transitions[Key(featureId, segmentIdx)] = transition;
}

bool CrossMwmConnector::IsTransition(Segment const & segment, bool isOutgoing) const
{
  auto it = m_transitions.find(Key(segment.GetFeatureId(), segment.GetSegmentIdx()));
  if (it == m_transitions.cend())
    return false;

  Transition const & transition = it->second;
  if (transition.m_oneWay && !segment.IsForward())
    return false;

  return (segment.IsForward() == transition.m_forwardIsEnter) == isOutgoing;
}

m2::PointD const & CrossMwmConnector::GetPoint(Segment const & segment, bool front) const
{
  Transition const & transition = GetTransition(segment);
  return segment.IsForward() == front ? transition.m_frontPoint : transition.m_backPoint;
}

void CrossMwmConnector::GetEdgeList(Segment const & segment, bool isOutgoing,
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

bool CrossMwmConnector::WeightsWereLoaded() const
{
  switch (m_weightsLoadState)
  {
  case WeightsLoadState::Unknown:
  case WeightsLoadState::ReadyToLoad: return false;
  case WeightsLoadState::NotExists:
  case WeightsLoadState::Loaded: return true;
  }
}

std::string DebugPrint(CrossMwmConnector::WeightsLoadState state)
{
  switch (state)
  {
  case CrossMwmConnector::WeightsLoadState::Unknown: return "Unknown";
  case CrossMwmConnector::WeightsLoadState::ReadyToLoad: return "ReadyToLoad";
  case CrossMwmConnector::WeightsLoadState::NotExists: return "NotExists";
  case CrossMwmConnector::WeightsLoadState::Loaded: return "Loaded";
  }
}

void CrossMwmConnector::AddEdge(Segment const & segment, Weight weight,
                                std::vector<SegmentEdge> & edges) const
{
  if (weight != kNoRoute)
    edges.emplace_back(segment, static_cast<double>(weight));
}

CrossMwmConnector::Transition const & CrossMwmConnector::GetTransition(
    Segment const & segment) const
{
  auto it = m_transitions.find(Key(segment.GetFeatureId(), segment.GetSegmentIdx()));
  CHECK(it != m_transitions.cend(), ("Not a transition segment:", segment));
  return it->second;
}

CrossMwmConnector::Weight CrossMwmConnector::GetWeight(size_t enterIdx, size_t exitIdx) const
{
  ASSERT_LESS(enterIdx, m_enters.size(), ());
  ASSERT_LESS(exitIdx, m_exits.size(), ());

  size_t const i = enterIdx * m_exits.size() + exitIdx;
  ASSERT_LESS(i, m_weights.size(), ());
  return m_weights[i];
}
}  // namespace routing
