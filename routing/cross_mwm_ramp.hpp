#pragma once

#include "routing/segment.hpp"

#include "base/assert.hpp"

#include "geometry/point2d.hpp"

#include <cmath>
#include <limits>
#include <unordered_map>
#include <vector>

namespace routing
{
class CrossMwmRamp final
{
public:
  CrossMwmRamp(NumMwmId mwmId) : m_mwmId(mwmId) {}
  void AddTransition(uint32_t featureId, uint32_t segmentIdx, bool oneWay, bool forwardIsEnter,
                     m2::PointD const & backPoint, m2::PointD const & frontPoint);

  bool IsTransition(Segment const & segment, bool isOutgoing) const;
  m2::PointD const & GetPoint(Segment const & segment, bool front) const;
  void GetEdgeList(Segment const & segment, bool isOutgoing,
                   std::vector<SegmentEdge> & edges) const;

  std::vector<Segment> const & GetEnters() const { return m_enters; }
  std::vector<Segment> const & GetExits() const { return m_exits; }
  bool HasWeights() const { return !m_weights.empty(); }
  bool WeightsWereLoaded() const { return m_weightsWereLoaded; }
  template <typename CalcWeight>
  void FillWeights(CalcWeight && calcWeight)
  {
    CHECK(m_weights.empty(), ());
    m_weights.reserve(m_enters.size() * m_exits.size());
    for (size_t i = 0; i < m_enters.size(); ++i)
    {
      for (size_t j = 0; j < m_exits.size(); ++j)
      {
        double const weight = calcWeight(m_enters[i], m_exits[j]);
        m_weights.push_back(static_cast<Weight>(std::ceil(weight)));
      }
    }
  }

private:
  // This is internal type used for storing edges weights.
  // Weight is the time requred for the route to pass.
  // Weight is measured in seconds rounded upwards.
  using Weight = uint32_t;

  static Weight constexpr kNoRoute = 0;

  struct Key
  {
    Key() = default;

    Key(uint32_t featureId, uint32_t segmentIdx) : m_featureId(featureId), m_segmentIdx(segmentIdx)
    {
    }

    bool operator==(const Key & key) const
    {
      return m_featureId == key.m_featureId && m_segmentIdx == key.m_segmentIdx;
    }

    uint32_t m_featureId = 0;
    uint32_t m_segmentIdx = 0;
  };

  struct HashKey
  {
    size_t operator()(const Key & key) const
    {
      return std::hash<uint64_t>()((static_cast<uint64_t>(key.m_featureId) << 32) +
                                   static_cast<uint64_t>(key.m_segmentIdx));
    }
  };

  struct Transition
  {
    Transition() = default;

    Transition(uint32_t enterIdx, uint32_t exitIdx, bool oneWay, bool forwardIsEnter,
               m2::PointD const & backPoint, m2::PointD const & frontPoint)
      : m_enterIdx(enterIdx)
      , m_exitIdx(exitIdx)
      , m_backPoint(backPoint)
      , m_frontPoint(frontPoint)
      , m_oneWay(oneWay)
      , m_forwardIsEnter(forwardIsEnter)
    {
    }

    uint32_t m_enterIdx = 0;
    uint32_t m_exitIdx = 0;
    m2::PointD m_backPoint = {0.0, 0.0};
    m2::PointD m_frontPoint = {0.0, 0.0};
    bool m_oneWay = false;
    bool m_forwardIsEnter = false;
  };

  friend class CrossMwmRampSerializer;

  void AddEdge(Segment const & segment, Weight weight, std::vector<SegmentEdge> & edges) const;
  Transition const & GetTransition(Segment const & segment) const;
  Weight GetWeight(size_t enterIdx, size_t exitIdx) const;

  NumMwmId const m_mwmId;
  std::vector<Segment> m_enters;
  std::vector<Segment> m_exits;
  std::unordered_map<Key, Transition, HashKey> m_transitions;
  std::vector<Weight> m_weights;
  bool m_weightsWereLoaded = false;
};
}  // namespace routing
