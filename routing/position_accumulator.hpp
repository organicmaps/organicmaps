#pragma once

#include "geometry/point2d.hpp"

#include <queue>

namespace routing
{
/// \brief This class accumulates a track of some number of last gps positions.
/// The track is used to calculate the direction of an end user.
class PositionAccumulator
{
public:
  // Several last gps positions are accumulating in |PositionAccumulator::m_points|.
  // The last position (the last point from |m_points|) may be removed if after the removing it
  // the length of the all track will be more than |kMinTrackLengthM|.
  static double constexpr kMinTrackLengthM = 70.0;

  // All segments which are shorter than
  static double constexpr kMinValidSegmentLengthM = 10.0;
  // are not added to the track.

  // If a segment is longer then
  static double constexpr kMaxValidSegmentLengthM = 80.0;
  // all the content of |m_points| is removed the current point is added to track.

  void PushNextPoint(m2::PointD const & point);
  void Clear();

  /// \returns direction or {0.0, 0.0} if |m_points| is empty or contains only one item.
  m2::PointD GetDirection() const;

  // Getters for testing this class.
  std::deque<m2::PointD> const & GetPointsForTesting() const { return m_points; }
  double GetTrackLengthMForTesting() const { return m_trackLengthM; }

private:
  // There's the current position at the end. At the beginning there's the last position of the track.
  // The distance between them is more or equal than |kMinTrackLengthM| if there're enough points in
  // |m_points|.
  std::deque<m2::PointD> m_points;
  double m_trackLengthM = 0.0;
};
}  // routing
