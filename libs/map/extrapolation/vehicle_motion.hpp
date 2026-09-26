#pragma once

#include "platform/location.hpp"

#include <deque>
#include <limits>
#include <optional>

namespace extrapolation
{
// All times are seconds on one monotonic clock. GNSS bearing is the direction of motion,
// not the vehicle's body heading, so a reversal waits for a new GNSS fix.
class VehicleMotion
{
public:
  static constexpr double kMaxAgeSeconds = 2.0;
  static constexpr double kMaxSpeedMps = 75.0;

  void SetFix(location::GpsInfo const & fix, double timestamp, double now);
  bool SetSpeed(double speedMps, double timestamp, double now);
  void InvalidateSpeed();
  std::optional<location::GpsInfo> Predict(double now);
  bool ShouldHoldPosition() const { return m_vehicleForFix; }

private:
  struct Speed
  {
    double m_time;
    double m_value;
  };
  bool HasFreshSpeed(double now) const;
  bool CanUseCourse() const;

  std::deque<Speed> m_speeds;
  double m_lastSpeedTime = -std::numeric_limits<double>::infinity();
  location::GpsInfo m_fix;
  double m_fixTime = 0.0;
  double m_distance = 0.0;
  bool m_fixValid = false;
  bool m_vehicleForFix = false;
  bool m_blockedUntilFix = false;
};
}  // namespace extrapolation
