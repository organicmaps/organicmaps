#include "routing/position_accumulator.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

#include <algorithm>

namespace routing
{
double constexpr PositionAccumulator::kMinTrackLengthM;
double constexpr PositionAccumulator::kMinValidSegmentLengthM;
double constexpr PositionAccumulator::kMaxValidSegmentLengthM;

void PositionAccumulator::PushNextPoint(m2::PointD const & point)
{
  double const lenM = m_points.empty() ? 0.0 : mercator::DistanceOnEarth(point, m_points.back());

  // If the last segment is too long it tells nothing about an end user direction.
  // And the history is not actual.
  if (lenM > kMaxValidSegmentLengthM)
  {
    Clear();
    m_points.push_back(point);
    return;
  }

  // If the last segment is too short it means an end user stays we there's no information
  // about it's direction. If m_points.empty() == true it means |point| is the first point.
  if (!m_points.empty() && lenM < kMinValidSegmentLengthM)
    return;

  // If |m_points| is empty |point| should be added any way.
  // If the size of |m_points| is 1 and |lenM| is valid |point| should be added.
  if (m_points.size() < 2)
  {
    CHECK_EQUAL(m_trackLengthM, 0.0, ());
    m_trackLengthM = lenM;
    m_points.push_back(point);
    return;
  }

  // If after adding |point| to |m_points| and removing the farthest point the segment length
  // is less than |kMinTrackLengthM| we just adding |point|.
  double oldestSegmentLenM = mercator::DistanceOnEarth(m_points[1], m_points[0]);
  if (m_trackLengthM + lenM - oldestSegmentLenM <= kMinTrackLengthM)
  {
    m_trackLengthM += lenM;
    m_points.push_back(point);
    return;
  }

  // Removing the farthest point if length of the track |m_points[1]|, ..., |m_points.back()|, |point|
  // is more than |kMinTrackLengthM|.
  while (m_trackLengthM + lenM - oldestSegmentLenM > kMinTrackLengthM && m_points.size() > 2)
  {
    m_trackLengthM -= oldestSegmentLenM;
    m_points.pop_front();
    oldestSegmentLenM = mercator::DistanceOnEarth(m_points[1], m_points[0]);
  }

  m_trackLengthM += lenM;
  m_points.push_back(point);
}

void PositionAccumulator::Clear()
{
  m_points.clear();
  m_trackLengthM = 0.0;
}

m2::PointD PositionAccumulator::GetDirection() const
{
  if (m_points.size() <= 1)
    return m2::PointD::Zero();

  return m_points.back() - m_points.front();
}
}  // namespace routing
