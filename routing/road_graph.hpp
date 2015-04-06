#pragma once

#include "../geometry/point2d.hpp"

#include "../base/string_utils.hpp"

#include "../std/vector.hpp"


namespace routing
{

class Route;

/// Defines position on a feature with direction.
class RoadPos
{
public:
  RoadPos() : m_featureId(0), m_pointId(0), m_endCoordinates(0, 0) {}
  RoadPos(uint32_t featureId, bool bForward, size_t pointId, m2::PointD const & p = m2::PointD::Zero());

  uint32_t GetFeatureId() const { return m_featureId >> 1; }
  bool IsForward() const { return (m_featureId & 1) != 0; }
  uint32_t GetPointId() const { return m_pointId; }

  bool operator==(RoadPos const & r) const
  {
    return (m_featureId == r.m_featureId && m_pointId == r.m_pointId);
  }

  bool operator < (RoadPos const & r) const
  {
    if (m_featureId != r.m_featureId)
      return m_featureId < r.m_featureId;
    if (m_pointId != r.m_pointId)
      return m_pointId < r.m_pointId;
    return false;
  }

  m2::PointD const & GetEndCoordinates() const { return m_endCoordinates; }

private:
  friend string DebugPrint(RoadPos const & r);

  uint32_t m_featureId;
  uint32_t m_pointId;
  m2::PointD m_endCoordinates;
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

  virtual ~IRoadGraph() {}

  /// Find all feature sections (turns), that route to the "pos" section.
  virtual void GetPossibleTurns(RoadPos const & pos, TurnsVectorT & turns, bool noOptimize = true) = 0;
  /// Construct route by road positions (doesn't include first and last section).
  virtual void ReconstructPath(RoadPosVectorT const & positions, Route & route) = 0;
};

} // namespace routing
