#include "map/nogps/gps_return_detector.hpp"

#include "map/nogps/geo.hpp"

#include <algorithm>
#include <cmath>

namespace nogps
{
namespace
{
bool IsSameSpeed(std::optional<double> gpsSpeedMps, std::optional<double> carSpeedMps)
{
  if (!gpsSpeedMps || !carSpeedMps)
    return false;
  return std::fabs(*gpsSpeedMps - *carSpeedMps) <=
         std::max(GpsReturnDetector::kMaxSpeedDiffMps, GpsReturnDetector::kMaxSpeedDiffRatio * *carSpeedMps);
}
}  // namespace

bool GpsReturnDetector::OnGpsPosition(ms::LatLon const & position, double accuracyM, int64_t timeMs,
                                      std::optional<Own> const & own, std::optional<double> gpsSpeedMps,
                                      std::optional<double> carSpeedMps, bool onRoad)
{
  // Positions from several providers come together, a rough one tells nothing, and GPS is lost if only rough
  // ones come: the gap breaks the streak then.
  if (accuracyM <= 0 || accuracyM > kMaxAccuracyM)
    return false;

  bool continues = false;
  if (m_last)
  {
    int64_t const gapMs = timeMs - m_last->m_timeMs;
    continues =
        gapMs >= 0 && gapMs <= kMaxGapMs &&
        Distance(position, m_last->m_position) <= kMaxSpeedMps * gapMs / 1000.0 + accuracyM + m_last->m_accuracyM;
  }
  if (!continues)
  {
    m_streakStartMs = timeMs;
    m_followStartMs = timeMs;
    m_followDistanceM = 0;
  }
  else
  {
    m_followDistanceM += Distance(position, m_last->m_position);
  }
  if (!onRoad || !IsSameSpeed(gpsSpeedMps, carSpeedMps))
  {
    m_followStartMs = timeMs;
    m_followDistanceM = 0;
  }

  m_last = Last{timeMs, position, accuracyM};

  if (timeMs - m_followStartMs >= kMinFollowMs && m_followDistanceM >= kMinFollowDistanceM)
    return true;
  if (own && Distance(position, own->m_position) > kMinToleranceM + accuracyM + own->m_errorM)
  {
    m_streakStartMs = timeMs;
    return false;
  }
  return timeMs - m_streakStartMs >= kMinStreakMs;
}
}  // namespace nogps
