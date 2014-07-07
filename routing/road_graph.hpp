#pragma once

#include "../geometry/point2d.hpp"

#include "../base/assert.hpp"
#include "../base/string_utils.hpp"

#include "../std/vector.hpp"


namespace routing
{

/// Defines position on a feature with direction.
class RoadPos
{
public:
  RoadPos() : m_featureId(0), m_pointId(0) {}
  RoadPos(uint32_t featureId, bool bForward, size_t pointId)
    : m_featureId((featureId << 1) + (bForward ? 1 : 0)), m_pointId(pointId)
  {
    ASSERT_LESS(featureId, 1U << 31, ());
    ASSERT_LESS(pointId, 0xFFFFFFFF, ());
  }

  uint32_t GetFeatureId() const { return m_featureId >> 1; }
  bool IsForward() const { return (m_featureId & 1) != 0; }
  uint32_t GetPointId() const { return m_pointId; }

  bool operator==(RoadPos const & r) const
  {
    return (m_featureId == r.m_featureId && m_pointId == r.m_pointId);
  }

private:
  friend string DebugPrint(RoadPos const & r);

  uint32_t m_featureId;
  uint32_t m_pointId;
};

/// The turn from the old to the new road.
struct PossibleTurn
{
  /// New road information.
  RoadPos m_pos;
  m2::PointD m_startPoint, m_endPoint;
  double m_speed;

  /// Distance and time to get to this turn on old road.
  double m_metersCovered;
  double m_secondsCovered;

  PossibleTurn() : m_metersCovered(0.0), m_secondsCovered(0.0) {}
};

inline string DebugPrint(PossibleTurn const & r)
{
  return DebugPrint(r.m_pos);
}

class IRoadGraph
{
public:
  typedef vector<PossibleTurn> TurnsVectorT;
  typedef vector<RoadPos> RoadPosVectorT;
  typedef vector<m2::PointD> PointsVectorT;

  /// Find all line feature sections, that follow the to pos section.
  virtual void GetPossibleTurns(RoadPos const & pos, TurnsVectorT & turns) = 0;
  /// Calculate distance in meters between two RoadPos that placed on the same feature
  virtual double GetFeatureDistance(RoadPos const & p1, RoadPos const & p2) = 0;
  /// Construct full path by road positions
  virtual void ReconstructPath(RoadPosVectorT const & positions, PointsVectorT & poly) = 0;
};

} // namespace routing
