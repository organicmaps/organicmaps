#pragma once

#include "routing/segment.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <cmath>
#include <limits>
#include <unordered_map>
#include <vector>

namespace routing
{
class CrossMwmConnector final
{
public:
  static double constexpr kNoRoute = 0.0;

  CrossMwmConnector() : m_mwmId(kFakeNumMwmId) {}
  explicit CrossMwmConnector(NumMwmId mwmId) : m_mwmId(mwmId) {}

  void AddTransition(uint64_t osmId, uint32_t featureId, uint32_t segmentIdx, bool oneWay,
                     bool forwardIsEnter, m2::PointD const & backPoint,
                     m2::PointD const & frontPoint);

  bool IsTransition(Segment const & segment, bool isOutgoing) const;
  uint64_t GetOsmId(Segment const & segment) const { return GetTransition(segment).m_osmId; }
  // returns nullptr if there is no transition for such osm id.
  Segment const * GetTransition(uint64_t osmId, uint32_t segmentIdx, bool isEnter) const;
  m2::PointD const & GetPoint(Segment const & segment, bool front) const;
  void GetEdgeList(Segment const & segment, bool isOutgoing,
                   std::vector<SegmentEdge> & edges) const;

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
  bool WeightsWereLoaded() const;

  template <typename CalcWeight>
  void FillWeights(CalcWeight && calcWeight)
  {
    CHECK_EQUAL(m_weightsLoadState, WeightsLoadState::Unknown, ());
    CHECK(m_weights.empty(), ());

    m_weights.reserve(m_enters.size() * m_exits.size());
    for (Segment const & enter : m_enters)
    {
      for (Segment const & exit : m_exits)
      {
        auto const weight = calcWeight(enter, exit);
        // Edges weights should be >= astar heuristic, so use std::ceil.
        m_weights.push_back(static_cast<Weight>(std::ceil(weight)));
      }
    }
  }

private:
  // This is an internal type for storing edges weights.
  // Weight is the time requred for the route to pass.
  // Weight is measured in seconds rounded upwards.
  using Weight = uint32_t;

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

  struct Transition
  {
    Transition() = default;

    Transition(uint32_t enterIdx, uint32_t exitIdx, uint64_t osmId, bool oneWay,
               bool forwardIsEnter, m2::PointD const & backPoint, m2::PointD const & frontPoint)
      : m_enterIdx(enterIdx)
      , m_exitIdx(exitIdx)
      , m_osmId(osmId)
      , m_backPoint(backPoint)
      , m_frontPoint(frontPoint)
      , m_oneWay(oneWay)
      , m_forwardIsEnter(forwardIsEnter)
    {
    }

    uint32_t m_enterIdx = 0;
    uint32_t m_exitIdx = 0;
    uint64_t m_osmId = 0;
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

  enum class WeightsLoadState
  {
    Unknown,
    NotExists,
    ReadyToLoad,
    Loaded
  };

  friend class CrossMwmConnectorSerializer;
  friend std::string DebugPrint(WeightsLoadState state);

  void AddEdge(Segment const & segment, Weight weight, std::vector<SegmentEdge> & edges) const;
  Transition const & GetTransition(Segment const & segment) const;
  Weight GetWeight(size_t enterIdx, size_t exitIdx) const;

  NumMwmId const m_mwmId;
  std::vector<Segment> m_enters;
  std::vector<Segment> m_exits;
  std::unordered_map<Key, Transition, HashKey> m_transitions;
  std::unordered_map<uint64_t, uint32_t> m_osmIdToFeatureId;
  WeightsLoadState m_weightsLoadState = WeightsLoadState::Unknown;
  uint64_t m_weightsOffset = 0;
  Weight m_granularity = 0;
  std::vector<Weight> m_weights;
};
}  // namespace routing
