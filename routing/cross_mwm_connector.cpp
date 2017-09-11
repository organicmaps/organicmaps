#include "routing/cross_mwm_connector.hpp"

#include "geometry/mercator.hpp"

namespace
{
uint32_t constexpr kFakeId = std::numeric_limits<uint32_t>::max();
}  // namespace

namespace routing
{
// static
double constexpr CrossMwmConnector::kNoRoute;

void CrossMwmConnector::AddTransition(uint64_t osmId, uint32_t featureId, uint32_t segmentIdx,
                                      bool oneWay, bool forwardIsEnter,
                                      m2::PointD const & backPoint, m2::PointD const & frontPoint)
{
  Transition transition(kFakeId, kFakeId, osmId, oneWay, forwardIsEnter, backPoint, frontPoint);

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
  m_osmIdToFeatureId.emplace(osmId, featureId);
}

bool CrossMwmConnector::IsTransition(Segment const & segment, bool isOutgoing) const
{
  auto it = m_transitions.find(Key(segment.GetFeatureId(), segment.GetSegmentIdx()));
  if (it == m_transitions.cend())
    return false;

  Transition const & transition = it->second;
  if (transition.m_oneWay && !segment.IsForward())
    return false;

  // Note. If |isOutgoing| == true |segment| should be an exit transition segment
  // (|isEnter| == false) to be a transition segment.
  // Otherwise |segment| should be an enter transition segment (|isEnter| == true)
  // to be a transition segment. If not, |segment| is not a transition segment.
  // Please see documentation on CrossMwmGraph::IsTransition() method for details.
  bool const isEnter = (segment.IsForward() == transition.m_forwardIsEnter);
  return isEnter != isOutgoing;
}

Segment const * CrossMwmConnector::GetTransition(uint64_t osmId, uint32_t segmentIdx,
                                                 bool isEnter) const
{
  auto fIt = m_osmIdToFeatureId.find(osmId);
  if (fIt == m_osmIdToFeatureId.cend())
    return nullptr;

  uint32_t const featureId = fIt->second;

  auto tIt = m_transitions.find(Key(featureId, segmentIdx));
  if (tIt == m_transitions.cend())
    return nullptr;

  Transition const & transition = tIt->second;
  CHECK_EQUAL(transition.m_osmId, osmId,
              ("feature:", featureId, ", segment:", segmentIdx, ", point:",
               MercatorBounds::ToLatLon(transition.m_frontPoint)));
  bool const isForward = transition.m_forwardIsEnter == isEnter;
  if (transition.m_oneWay && !isForward)
    return nullptr;

  Segment const & segment =
      isEnter ? GetEnter(transition.m_enterIdx) : GetExit(transition.m_exitIdx);
  CHECK_EQUAL(segment.IsForward(), isForward, ("osmId:", osmId, ", segment:", segment, ", point:",
                                               MercatorBounds::ToLatLon(transition.m_frontPoint)));
  return &segment;
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
    edges.emplace_back(segment, RouteWeight::FromCrossMwmWeight(weight));
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
