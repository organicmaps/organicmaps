#pragma once

#include "geometry/point2d.hpp"

namespace df
{
struct GpsTrackPoint
{
  // Timestamp of the point, seconds from 1st Jan 1970
  double m_timestamp;

  // Point in the Mercator projection
  m2::PointD m_point;

  // Speed in the point, M/S
  double m_speedMPS;

  // Unique identifier of the point
  uint32_t m_id;
};
}  // namespace df
