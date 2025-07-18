#pragma once

#include "geometry/point2d.hpp"

#include <cstdint>
#include <limits>

namespace routing
{
enum class SpeedCameraManagerMode;

struct SpeedCameraOnRoute
{
  SpeedCameraOnRoute() = default;
  SpeedCameraOnRoute(double distFromBegin, uint8_t maxSpeedKmH, m2::PointD const & position)
    : m_distFromBeginMeters(distFromBegin), m_maxSpeedKmH(maxSpeedKmH), m_position(position)
  {}

  static uint8_t constexpr kNoSpeedInfo = std::numeric_limits<uint8_t>::max();

  bool NoSpeed() const;

  bool IsValid() const;
  void Invalidate();

  double m_distFromBeginMeters = 0.0;    // Distance from beginning of route to current camera.
  uint8_t m_maxSpeedKmH = kNoSpeedInfo;  // Maximum speed allowed by the camera.
  m2::PointD m_position = m2::PointD::Max();
};
}  // namespace routing
