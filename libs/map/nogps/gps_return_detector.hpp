#pragma once

#include "geometry/latlon.hpp"

#include <cstdint>
#include <optional>

namespace nogps
{
/// Decides when GPS can be used again after the manual mode, which is turned on when GPS is jammed or
/// spoofed. Not being spoofed is not enough: a spoofer can move the position a little only, and the spoofing
/// detector compares it with the coarse cell tower positions. GPS is trusted when it has given accurate
/// positions for a while without a break or a jump, and they agree with the position known without GPS: the
/// mark set by the user or the inertial one.
/// The own position can be wrong too, e.g. a mark set in a wrong place. GPS far from it is trusted when it
/// follows the car for a long drive: it moves along the roads as fast as the car does. A spoofer can't move
/// the position along with the car.
class GpsReturnDetector
{
public:
  // Spoofing and jamming come and go, GPS must work for a while before the manual mode is left.
  static int64_t constexpr kMinStreakMs = 15'000;
  // GPS gives a position every second, a longer pause is a break of the streak.
  static int64_t constexpr kMaxGapMs = 3000;
  static double constexpr kMaxAccuracyM = 25;
  // Faster than any car on Ukrainian roads, used to estimate how far the car could move.
  static double constexpr kMaxSpeedMps = 180 / 3.6;
  // GPS and the own position are compared with this margin in addition to their errors.
  static double constexpr kMinToleranceM = 100;
  // GPS follows the car when it agrees with the car speed for so long and so far. The distance tells a moving
  // car from a standing one, it is driven in a minute in a jam.
  static int64_t constexpr kMinFollowMs = 30'000;
  static double constexpr kMinFollowDistanceM = 100;
  static double constexpr kMaxSpeedDiffMps = 2;
  static double constexpr kMaxSpeedDiffRatio = 0.2;

  /// The position known without GPS and how far the car can be from it.
  struct Own
  {
    ms::LatLon m_position;
    double m_errorM = 0;
  };

  void Reset() { m_last.reset(); }

  /// Checks a GPS position trusted by the spoofing detector.
  /// \param timeMs monotonic time of the position, e.g. elapsed realtime.
  /// \param own the position known without GPS, if there is one.
  /// \param gpsSpeedMps the speed told by GPS.
  /// \param carSpeedMps the speed of the car from its sensors.
  /// \param onRoad true if the position is on a road.
  /// \returns true if GPS can be used again.
  bool OnGpsPosition(ms::LatLon const & position, double accuracyM, int64_t timeMs, std::optional<Own> const & own,
                     std::optional<double> gpsSpeedMps, std::optional<double> carSpeedMps, bool onRoad);

private:
  struct Last
  {
    int64_t m_timeMs;
    ms::LatLon m_position;
    double m_accuracyM;
  };

  int64_t m_streakStartMs = 0;
  int64_t m_followStartMs = 0;
  double m_followDistanceM = 0;
  std::optional<Last> m_last;
};
}  // namespace nogps
