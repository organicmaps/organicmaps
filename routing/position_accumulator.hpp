#pragma once

#include "geometry/point2d.hpp"

#include <queue>

/// \brief This class accumulates a track of some number of last gps positions.
/// The track is used to calculate the direction of an end user.
class PositionAccumulator
{
public:
  // Several last gps positions are accumulating in |PositionAccumulator::m_points|.
  // The last positions may be removed from |PositionAccumulator::m_points| in several cases.
  // 1. If after the removing the last point the length of the track will be more than |kMinTrackLengthM|.
  static double constexpr kMinTrackLengthM = 70.0;
  // 2. All points from |m_points| except for |m_points.back()| if the distance between
  // |m_points.back()| and next point (|point|) is more than
  static double constexpr kMinGoodSegmentLengthM = 9.0;
  // and more than
  static double constexpr kMaxGoodSegmentLengthM = 80.0;
  // Than means that only last segment is used to get the direction
  // |PositionAccumulator::GetDirection()| if a driver goes quick enough.

  // All segments which are shorter than
  static double constexpr kMinValidSegmentLengthM = 1.0;
  // and longer than
  static double constexpr kMaxValidSegmentLengthM = kMaxGoodSegmentLengthM;
  // should be removed.

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
