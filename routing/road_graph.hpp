#pragma once

#include "geometry/point2d.hpp"

#include "base/string_utils.hpp"

#include "std/vector.hpp"

namespace routing
{
class Route;

/// Defines position on a feature which represents a road with direction.
class RoadPos
{
public:
  RoadPos() : m_featureId(0), m_segId(0), m_segEndpoint(0, 0) {}
  RoadPos(uint32_t featureId, bool forward, size_t segId,
          m2::PointD const & p = m2::PointD::Zero());

  uint32_t GetFeatureId() const { return m_featureId >> 1; }
  bool IsForward() const { return (m_featureId & 1) != 0; }
  uint32_t GetSegId() const { return m_segId; }
  uint32_t GetSegStartPointId() const { return m_segId + (IsForward() ? 0 : 1); }
  uint32_t GetSegEndPointId() const { return m_segId + (IsForward() ? 1 : 0); }
  m2::PointD const & GetSegEndpoint() const { return m_segEndpoint; }

  bool operator==(RoadPos const & r) const
  {
    return (m_featureId == r.m_featureId && m_segId == r.m_segId);
  }

  bool operator!=(RoadPos const & r) const
  {
    return (m_featureId != r.m_featureId || m_segId != r.m_segId);
  }

  bool operator<(RoadPos const & r) const
  {
    if (m_featureId != r.m_featureId)
      return m_featureId < r.m_featureId;
    if (m_segId != r.m_segId)
      return m_segId < r.m_segId;
    return false;
  }

private:
  friend string DebugPrint(RoadPos const & r);

  // Feature on which position is defined.
  uint32_t m_featureId;

  // Ordinal number of the segment on the road.
  uint32_t m_segId;

  // End-point of the segment on the road.
  m2::PointD m_segEndpoint;
};

/// The turn from the old to the new road.
struct PossibleTurn
{
  /// New road information.
  RoadPos m_pos;

  /// Start point on the old road.
  m2::PointD m_startPoint;

  /// End point on the old road.
  m2::PointD m_endPoint;

  /// Speed on the old road.
  double m_speed;

  /// Distance and time to get to this turn on old road.
  double m_metersCovered;
  double m_secondsCovered;

  PossibleTurn() : m_metersCovered(0.0), m_secondsCovered(0.0) {}
};

inline string DebugPrint(PossibleTurn const & r) { return DebugPrint(r.m_pos); }

class IRoadGraph
{
public:
  typedef vector<PossibleTurn> TurnsVectorT;
  typedef vector<RoadPos> RoadPosVectorT;
  typedef vector<m2::PointD> PointsVectorT;

  virtual ~IRoadGraph() {}

  /// Finds all nearest feature sections (turns), that route to the
  /// "pos" section.
  virtual void GetNearestTurns(RoadPos const & pos, TurnsVectorT & turns) = 0;

  /// Construct route by road positions (doesn't include first and last section).
  virtual void ReconstructPath(RoadPosVectorT const & positions, Route & route) = 0;
};

}  // namespace routing
