#include "map/extrapolation/extrapolator.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include <chrono>
#include <memory>

namespace
{
// If the difference of an value between two instances of GpsInfo is greater
// than the appropriate constant below the extrapolator will be switched off for
// these two instances of GpsInfo.
double constexpr kMaxExtrapolationSpeedMPS = 120.0;
double constexpr kMaxExtrapolationDistMeters = 120.0;
double constexpr kMaxExtrapolationTimeSeconds = 2.1;

class LinearExtrapolator
{
public:
  LinearExtrapolator(uint64_t timeBetweenMs, uint64_t timeAfterMs)
    : m_timeBetweenMs(timeBetweenMs), m_timeAfterMs(timeAfterMs)
  {
    CHECK_NOT_EQUAL(m_timeBetweenMs, 0, ());
  }

  double Extrapolate(double x1, double x2)
  {
    return x2 + ((x2 - x1) / m_timeBetweenMs) * m_timeAfterMs;
  }

private:
  uint64_t m_timeBetweenMs;
  uint64_t m_timeAfterMs;
};
}  // namespace

namespace extrapolation
{
using namespace std;

location::GpsInfo LinearExtrapolation(location::GpsInfo const & gpsInfo1,
                                      location::GpsInfo const & gpsInfo2,
                                      uint64_t timeAfterPoint2Ms)
{
  CHECK_LESS(gpsInfo1.m_timestamp, gpsInfo2.m_timestamp, ());
  auto const timeBetweenPointsMs =
      static_cast<uint64_t>((gpsInfo2.m_timestamp - gpsInfo1.m_timestamp) * 1000);

  location::GpsInfo result = gpsInfo2;
  LinearExtrapolator e(timeBetweenPointsMs, timeAfterPoint2Ms);

  result.m_timestamp += timeAfterPoint2Ms;
  result.m_longitude = e.Extrapolate(gpsInfo1.m_longitude, gpsInfo2.m_longitude);
  result.m_latitude = e.Extrapolate(gpsInfo1.m_latitude, gpsInfo2.m_latitude);
  result.m_horizontalAccuracy = e.Extrapolate(gpsInfo1.m_horizontalAccuracy, gpsInfo2.m_horizontalAccuracy);
  result.m_altitude = e.Extrapolate(gpsInfo1.m_altitude, gpsInfo2.m_altitude);

  if (gpsInfo1.HasVerticalAccuracy() && gpsInfo2.HasVerticalAccuracy())
    result.m_verticalAccuracy = e.Extrapolate(gpsInfo1.m_verticalAccuracy, gpsInfo2.m_verticalAccuracy);

  // @TODO(bykoianko) Now |result.m_bearing == gpsInfo2.m_bearing|.
  // In case of |gpsInfo1.HasBearing() && gpsInfo2.HasBearing() == true|
  // consider finding an average value between |gpsInfo1.m_bearing| and |gpsInfo2.m_bearing|
  // taking into account that they are periodic. It's important to implement it
  // because current implementation leads to changing course by steps. It doesn't
  // look nice when the road changes its direction.

  if (gpsInfo1.HasSpeed() && gpsInfo2.HasSpeed())
    result.m_speed = e.Extrapolate(gpsInfo1.m_speed, gpsInfo2.m_speed);

  return result;
}

bool AreCoordsGoodForExtrapolation(location::GpsInfo const & beforeLastGpsInfo,
                                   location::GpsInfo const & lastGpsInfo)
{
  double const distM =
      ms::DistanceOnEarth(beforeLastGpsInfo.m_latitude, beforeLastGpsInfo.m_longitude,
                          lastGpsInfo.m_latitude, lastGpsInfo.m_longitude);
  double const timeS = lastGpsInfo.m_timestamp - beforeLastGpsInfo.m_timestamp;

  // Note. |timeS| may be less than zero. (beforeLastGpsInfo.m_timestamp >= lastGpsInfo.m_timestamp)
  // It may happen in rare cases because GpsInfo::m_timestamp is not monotonic generally.
  // Please see comment in declaration of class GpsInfo for details.

  // @TODO(bykoianko) Switching off extrapolation based on acceleration should be implemented.
  // Switching off extrapolation based on speed, distance and time.
  return distM / timeS <= kMaxExtrapolationSpeedMPS && distM <= kMaxExtrapolationDistMeters &&
         timeS <= kMaxExtrapolationTimeSeconds && timeS > 0;
}

// Extrapolator ------------------------------------------------------------------------------------
Extrapolator::Extrapolator(ExtrapolatedLocationUpdateFn const & update)
  : m_isEnabled(false), m_extrapolatedLocationUpdate(update)
{
  GetPlatform().RunTask(Platform::Thread::Background, [this]
  {
    ExtrapolatedLocationUpdate();
  });
}

void Extrapolator::OnLocationUpdate(location::GpsInfo const & gpsInfo)
{
  // @TODO Consider calling ExtrapolatedLocationUpdate() on background thread immediately
  // after OnLocationUpdate() was called.
  lock_guard<mutex> guard(m_mutex);
  m_beforeLastGpsInfo = m_lastGpsInfo;
  m_lastGpsInfo = gpsInfo;
  m_extrapolationCounter = 0;
}

void Extrapolator::Enable(bool enabled)
{
  lock_guard<mutex> guard(m_mutex);
  m_isEnabled = enabled;
}

void Extrapolator::ExtrapolatedLocationUpdate()
{
  location::GpsInfo gpsInfo;
  do
  {
    lock_guard<mutex> guard(m_mutex);
    uint64_t const extrapolationTimeMs = kExtrapolationPeriodMs * m_extrapolationCounter;
    if (extrapolationTimeMs >= kMaxExtrapolationTimeMs || !m_lastGpsInfo.IsValid())
      break;

    if (DoesExtrapolationWork())
    {
      gpsInfo = LinearExtrapolation(m_beforeLastGpsInfo, m_lastGpsInfo, extrapolationTimeMs);
      break;
    }

    if (m_lastGpsInfo.IsValid())
      gpsInfo = m_lastGpsInfo;
  } while (false);

  if (gpsInfo.IsValid())
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [this, gpsInfo]() {
      m_extrapolatedLocationUpdate(gpsInfo);
    });
  }

  {
    lock_guard<mutex> guard(m_mutex);
    if (m_extrapolationCounter != kExtrapolationCounterUndefined)
      ++m_extrapolationCounter;
  }

  // Call ExtrapolatedLocationUpdate() every |kExtrapolationPeriodMs| milliseconds.
  auto constexpr kSExtrapolationPeriod = std::chrono::milliseconds(kExtrapolationPeriodMs);
  GetPlatform().RunDelayedTask(Platform::Thread::Background, kSExtrapolationPeriod, [this]
  {
    ExtrapolatedLocationUpdate();
  });
}

bool Extrapolator::DoesExtrapolationWork() const
{
  if (!m_isEnabled || m_extrapolationCounter == kExtrapolationCounterUndefined ||
      !m_lastGpsInfo.IsValid() || !m_beforeLastGpsInfo.IsValid())
  {
    return false;
  }

  return AreCoordsGoodForExtrapolation(m_beforeLastGpsInfo, m_lastGpsInfo);
}
}  // namespace extrapolation
