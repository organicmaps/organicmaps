#pragma once

#include "../geometry/point2d.hpp"

#include "../base/assert.hpp"

#include "../std/vector.hpp"


namespace routing
{

/// Defines position on a feature with direction.
class RoadPos
{
public:
  RoadPos(uint32_t featureId, bool bForward, size_t pointId)
    : m_featureId((featureId << 1) + (bForward ? 1 : 0)), m_pointId(pointId)
  {
    ASSERT_LESS(featureId, 1U << 31, ());
    ASSERT_LESS(pointId, 0xFFFFFFFF, ());
  }

  uint32_t GetFeatureId() const { return m_featureId >> 1; }
  bool IsForward() const { return (m_featureId & 1) != 0; }
  uint32_t GetPointId() const { return m_pointId; }

private:
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
};

class IRoadGraph
{
public:
  virtual void GetPossibleTurns(RoadPos const & pos, vector<PossibleTurn> & turns) = 0;
};

}
