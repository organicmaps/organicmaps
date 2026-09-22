#include "map/extrapolation/vehicle_motion.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <cmath>

namespace extrapolation
{
bool VehicleMotion::HasFreshSpeed(double now) const
{
  return !m_speeds.empty() && now >= m_speeds.back().m_time && now - m_speeds.back().m_time <= kMaxAgeSeconds;
}

bool VehicleMotion::CanUseCourse() const
{
  return std::isfinite(m_fix.m_bearing) && m_fix.m_bearing >= 0.0 && m_fix.m_bearing < 360.0;
}

void VehicleMotion::SetFix(location::GpsInfo const & fix, double timestamp, double now)
{
  m_fix = fix;
  m_fixTime = timestamp;
  m_distance = 0.0;
  m_blockedUntilFix = false;
  m_fixValid = fix.IsValid() && std::isfinite(timestamp) && timestamp <= now && now - timestamp < kMaxAgeSeconds &&
               std::isfinite(fix.m_timestamp) && std::isfinite(fix.m_latitude) && std::isfinite(fix.m_longitude) &&
               std::abs(fix.m_latitude) < 90.0 && std::abs(fix.m_longitude) <= 180.0;
  m_vehicleForFix = m_fixValid && HasFreshSpeed(now) && (CanUseCourse() || m_speeds.back().m_value == 0.0);
}

bool VehicleMotion::SetSpeed(double speedMps, double timestamp, double now)
{
  if (std::isfinite(timestamp) && timestamp <= m_lastSpeedTime)
    return false;
  if (!std::isfinite(speedMps) || std::abs(speedMps) > kMaxSpeedMps || !std::isfinite(timestamp) || timestamp > now ||
      now - timestamp > kMaxAgeSeconds)
  {
    InvalidateSpeed();
    return false;
  }
  m_lastSpeedTime = timestamp;
  m_speeds.push_back({timestamp, speedMps});
  // Retain a sample before the integration window, with a hard bound for misbehaving HALs.
  while (m_speeds.size() > 2 && m_speeds[1].m_time < now - 2.0 * kMaxAgeSeconds)
    m_speeds.pop_front();
  while (m_speeds.size() > 512)
    m_speeds.pop_front();
  if (m_fixValid && (CanUseCourse() || speedMps == 0.0))
    m_vehicleForFix = true;
  return true;
}

void VehicleMotion::InvalidateSpeed()
{
  m_speeds.clear();
  m_blockedUntilFix = m_vehicleForFix;
}

std::optional<location::GpsInfo> VehicleMotion::Predict(double now)
{
  if (!m_fixValid || !m_vehicleForFix || m_blockedUntilFix || now < m_fixTime || now - m_fixTime >= kMaxAgeSeconds ||
      !HasFreshSpeed(now))
    return {};

  size_t start = 0;
  while (start + 1 < m_speeds.size() && m_speeds[start + 1].m_time <= m_fixTime)
    ++start;
  if (m_speeds[start].m_time > m_fixTime || m_fixTime - m_speeds[start].m_time > kMaxAgeSeconds)
    return {};  // Never fill an unmeasured gap with a later speed sample.

  double distance = 0.0;
  double time = m_fixTime;
  double speed = m_speeds[start].m_value;
  int direction = speed > 0.0 ? 1 : (speed < 0.0 ? -1 : 0);
  for (size_t i = start + 1; i < m_speeds.size() && m_speeds[i].m_time <= now; ++i)
  {
    auto const & next = m_speeds[i];
    if (next.m_time - m_speeds[i - 1].m_time > kMaxAgeSeconds)
      return {};
    distance += std::abs(speed) * (next.m_time - time);
    int const nextDirection = next.m_value > 0.0 ? 1 : (next.m_value < 0.0 ? -1 : 0);
    if (direction != 0 && nextDirection != 0 && direction != nextDirection)
    {
      m_blockedUntilFix = true;
      return {};
    }
    if (nextDirection != 0)
      direction = nextDirection;
    time = next.m_time;
    speed = next.m_value;
  }
  distance += std::abs(speed) * (now - time);
  if (distance > 0.0 && !CanUseCourse())
    return {};
  // A delayed braking sample must not move the rendered position backwards.
  m_distance = std::max(m_distance, distance);

  location::GpsInfo result = m_fix;
  result.m_timestamp += now - m_fixTime;
  result.m_speed = std::abs(speed);
  if (m_distance > 0.0)
  {
    double const delta = m_distance / ms::kEarthRadiusMeters;
    double const latitude = math::DegToRad(m_fix.m_latitude);
    double const bearing = math::DegToRad(m_fix.m_bearing);
    double const sinLat = std::sin(latitude), cosLat = std::cos(latitude);
    double const resultLat =
        std::asin(std::clamp(sinLat * std::cos(delta) + cosLat * std::sin(delta) * std::cos(bearing), -1.0, 1.0));
    double const resultLon =
        math::DegToRad(m_fix.m_longitude) +
        std::atan2(std::sin(bearing) * std::sin(delta) * cosLat, std::cos(delta) - sinLat * std::sin(resultLat));
    result.m_latitude = math::RadToDeg(resultLat);
    result.m_longitude = std::remainder(math::RadToDeg(resultLon), 360.0);
  }
  return result;
}
}  // namespace extrapolation
