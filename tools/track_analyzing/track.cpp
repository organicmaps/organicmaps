#include "track_analyzing/track.hpp"

namespace track_analyzing
{
TrackFilter::TrackFilter(uint64_t minDuration, double minLength, double minSpeed, double maxSpeed, bool ignoreTraffic)
  : m_minDuration(minDuration)
  , m_minLength(minLength)
  , m_minSpeed(minSpeed)
  , m_maxSpeed(maxSpeed)
  , m_ignoreTraffic(ignoreTraffic)
{}

bool TrackFilter::Passes(uint64_t duration, double length, double speed, bool hasTrafficPoints) const
{
  if (duration < m_minDuration)
    return false;

  if (length < m_minLength)
    return false;

  if (speed < m_minSpeed || speed > m_maxSpeed)
    return false;

  return !(m_ignoreTraffic && hasTrafficPoints);
}
}  // namespace track_analyzing
