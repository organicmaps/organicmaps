#pragma once

#include "routing/cross_mwm_ids.hpp"
#include "routing/segment.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace routing
{
namespace connector
{
uint32_t constexpr kFakeIndex = std::numeric_limits<uint32_t>::max();
double constexpr kNoRoute = 0.0;

using Weight = uint32_t;

enum class WeightsLoadState
{
  Unknown,
  NotExists,
  ReadyToLoad,
  Loaded
};

std::string DebugPrint(WeightsLoadState state);
}  // namespace connector

template <typename CrossMwmId>
class CrossMwmConnector final
{
public:
  CrossMwmConnector() : m_mwmId(kFakeNumMwmId) {}
  CrossMwmConnector(NumMwmId mwmId, uint32_t featureNumerationOffset)
    : m_mwmId(mwmId), m_featureNumerationOffset(featureNumerationOffset)
  {
  }

  void AddTransition(CrossMwmId const & crossMwmId, uint32_t featureId, uint32_t segmentIdx, bool oneWay,
                     bool forwardIsEnter, m2::PointD const & backPoint,
                     m2::PointD const & frontPoint)
  {
    featureId += m_featureNumerationOffset;

    Transition<CrossMwmId> transition(connector::kFakeIndex, connector::kFakeIndex, crossMwmId,
                                      oneWay, forwardIsEnter, backPoint, frontPoint);

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
    m_crossMwmIdToFeatureId.emplace(crossMwmId, featureId);
  }

  bool IsTransition(Segment const & segment, bool isOutgoing) const
  {
    auto const it = m_transitions.find(Key(segment.GetFeatureId(), segment.GetSegmentIdx()));
    if (it == m_transitions.cend())
      return false;

    auto const & transition = it->second;
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

  CrossMwmId const & GetCrossMwmId(Segment const & segment) const
  {
    return GetTransition(segment).m_crossMwmId;
  }

  // returns nullptr if there is no transition for such cross mwm id.
  Segment const * GetTransition(CrossMwmId const & crossMwmId, uint32_t segmentIdx, bool isEnter) const
  {
    auto const fIt = m_crossMwmIdToFeatureId.find(crossMwmId);
    if (fIt == m_crossMwmIdToFeatureId.cend())
      return nullptr;

    uint32_t const featureId = fIt->second;

    auto const tIt = m_transitions.find(Key(featureId, segmentIdx));
    if (tIt == m_transitions.cend())
      return nullptr;

    auto const & transition = tIt->second;
    CHECK_EQUAL(transition.m_crossMwmId, crossMwmId,
                ("feature:", featureId, ", segment:", segmentIdx,
                 ", point:", MercatorBounds::ToLatLon(transition.m_frontPoint)));
    bool const isForward = transition.m_forwardIsEnter == isEnter;
    if (transition.m_oneWay && !isForward)
      return nullptr;

    Segment const & segment =
        isEnter ? GetEnter(transition.m_enterIdx) : GetExit(transition.m_exitIdx);
    CHECK_EQUAL(segment.IsForward(), isForward,
                ("crossMwmId:", crossMwmId, ", segment:", segment,
                 ", point:", MercatorBounds::ToLatLon(transition.m_frontPoint)));
    return &segment;
  }

  m2::PointD const & GetPoint(Segment const & segment, bool front) const
  {
    auto const & transition = GetTransition(segment);
    return segment.IsForward() == front ? transition.m_frontPoint : transition.m_backPoint;
  }

  void GetOutgoingEdgeList(Segment const & segment, std::vector<SegmentEdge> & edges) const
  {
    auto const & transition = GetTransition(segment);
    CHECK_NOT_EQUAL(transition.m_enterIdx, connector::kFakeIndex, ());
    for (size_t exitIdx = 0; exitIdx < m_exits.size(); ++exitIdx)
    {
      auto const weight = GetWeight(base::asserted_cast<size_t>(transition.m_enterIdx), exitIdx);
      AddEdge(m_exits[exitIdx], weight, edges);
    }
  }

  std::vector<Segment> const & GetEnters() const { return m_enters; }
  std::vector<Segment> const & GetExits() const { return m_exits; }  

  Segment const & GetEnter(size_t i) const
  {
    ASSERT_LESS(i, m_enters.size(), ());
    return m_enters[i];
  }

  Segment const & GetExit(size_t i) const
  {
    ASSERT_LESS(i, m_exits.size(), ());
    return m_exits[i];
  }

  bool HasWeights() const { return !m_weights.empty(); }
  bool IsEmpty() const { return m_enters.empty() && m_exits.empty(); }

  bool WeightsWereLoaded() const
  {
    switch (m_weightsLoadState)
    {
    case connector::WeightsLoadState::Unknown:
    case connector::WeightsLoadState::ReadyToLoad: return false;
    case connector::WeightsLoadState::NotExists:
    case connector::WeightsLoadState::Loaded: return true;
    }
    UNREACHABLE();
  }

  template <typename CalcWeight>
  void FillWeights(CalcWeight && calcWeight)
  {
    CHECK_EQUAL(m_weightsLoadState, connector::WeightsLoadState::Unknown, ());
    CHECK(m_weights.empty(), ());

    m_weights.reserve(m_enters.size() * m_exits.size());
    for (Segment const & enter : m_enters)
    {
      for (Segment const & exit : m_exits)
      {
        auto const weight = calcWeight(enter, exit);
        // Edges weights should be >= astar heuristic, so use std::ceil.
        m_weights.push_back(static_cast<connector::Weight>(std::ceil(weight)));
      }
    }
  }

private:
  struct Key
  {
    Key() = default;

    Key(uint32_t featureId, uint32_t segmentIdx) : m_featureId(featureId), m_segmentIdx(segmentIdx)
    {
    }

    bool operator==(Key const & key) const
    {
      return m_featureId == key.m_featureId && m_segmentIdx == key.m_segmentIdx;
    }

    uint32_t m_featureId = 0;
    uint32_t m_segmentIdx = 0;
  };

  struct HashKey
  {
    size_t operator()(Key const & key) const
    {
      return std::hash<uint64_t>()((static_cast<uint64_t>(key.m_featureId) << 32) +
                                   static_cast<uint64_t>(key.m_segmentIdx));
    }
  };

  template <typename CrossMwmIdInner>
  struct Transition
  {
    Transition() = default;

    Transition(uint32_t enterIdx, uint32_t exitIdx, CrossMwmIdInner const & crossMwmId, bool oneWay,
               bool forwardIsEnter, m2::PointD const & backPoint, m2::PointD const & frontPoint)
      : m_enterIdx(enterIdx)
      , m_exitIdx(exitIdx)
      , m_crossMwmId(crossMwmId)
      , m_backPoint(backPoint)
      , m_frontPoint(frontPoint)
      , m_oneWay(oneWay)
      , m_forwardIsEnter(forwardIsEnter)
    {
    }

    uint32_t m_enterIdx = 0;
    uint32_t m_exitIdx = 0;
    CrossMwmIdInner m_crossMwmId = CrossMwmIdInner();
    // Endpoints of transition segment.
    // m_backPoint = points[segmentIdx]
    // m_frontPoint = points[segmentIdx + 1]
    m2::PointD m_backPoint = m2::PointD::Zero();
    m2::PointD m_frontPoint = m2::PointD::Zero();
    bool m_oneWay = false;
    // Transition represents both forward and backward segments with same featureId, segmentIdx.
    // m_forwardIsEnter == true means: forward segment is enter to mwm:
    // Enter means: m_backPoint is outside mwm borders, m_frontPoint is inside.
    bool m_forwardIsEnter = false;
  };

  friend class CrossMwmConnectorSerializer;

  void AddEdge(Segment const & segment, connector::Weight weight,
               std::vector<SegmentEdge> & edges) const
  {
    // @TODO Double and uint32_t are compared below. This comparison should be fixed.
    if (weight != connector::kNoRoute)
      edges.emplace_back(segment, RouteWeight::FromCrossMwmWeight(weight));
  }

  CrossMwmConnector<CrossMwmId>::Transition<CrossMwmId> const & GetTransition(
      Segment const & segment) const
  {
    auto const it = m_transitions.find(Key(segment.GetFeatureId(), segment.GetSegmentIdx()));
    CHECK(it != m_transitions.cend(), ("Not a transition segment:", segment));
    return it->second;
  }

  connector::Weight GetWeight(size_t enterIdx, size_t exitIdx) const
  {
    ASSERT_LESS(enterIdx, m_enters.size(), ());
    ASSERT_LESS(exitIdx, m_exits.size(), ());

    size_t const i = enterIdx * m_exits.size() + exitIdx;
    ASSERT_LESS(i, m_weights.size(), ());
    return m_weights[i];
  }

  NumMwmId const m_mwmId;
  std::vector<Segment> m_enters;
  std::vector<Segment> m_exits;
  std::unordered_map<Key, Transition<CrossMwmId>, HashKey> m_transitions;
  std::unordered_map<CrossMwmId, uint32_t, connector::HashKey> m_crossMwmIdToFeatureId;
  connector::WeightsLoadState m_weightsLoadState = connector::WeightsLoadState::Unknown;
  // For some connectors we may need to shift features with some offset.
  // For example for versions and transit section compatibility we number transit features
  // starting from 0 in mwm and shift them with |m_featureNumerationOffset| in runtime.
  uint32_t const m_featureNumerationOffset = 0;
  uint64_t m_weightsOffset = 0;
  connector::Weight m_granularity = 0;
  // |m_weights| stores edge weights.
  // Weight is the time required for the route to pass edges.
  // Weight is measured in seconds rounded upwards.
  std::vector<connector::Weight> m_weights;
};
}  // namespace routing
