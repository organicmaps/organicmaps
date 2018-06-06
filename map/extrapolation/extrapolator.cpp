#include "map/extrapolation/extrapolator.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"

#include <chrono>
#include <memory>

namespace
{
// If the difference of values between two instances of GpsInfo is greater
// than the appropriate constant below the extrapolator will be switched off for
// these two instances of GpsInfo.
double constexpr kMaxExtrapolationSpeedMPS = 85.0;
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

  result.m_timestamp += static_cast<double>(timeAfterPoint2Ms) / 1000.0;
  result.m_longitude =
      my::clamp(e.Extrapolate(gpsInfo1.m_longitude, gpsInfo2.m_longitude), -180.0, 180.0);
  result.m_latitude =
      my::clamp(e.Extrapolate(gpsInfo1.m_latitude, gpsInfo2.m_latitude), -90.0, 90.0);
  result.m_horizontalAccuracy =
      e.Extrapolate(gpsInfo1.m_horizontalAccuracy, gpsInfo2.m_horizontalAccuracy);
  result.m_altitude = e.Extrapolate(gpsInfo1.m_altitude, gpsInfo2.m_altitude);

  if (gpsInfo1.HasVerticalAccuracy() && gpsInfo2.HasVerticalAccuracy())
    result.m_verticalAccuracy =
        e.Extrapolate(gpsInfo1.m_verticalAccuracy, gpsInfo2.m_verticalAccuracy);

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
  if (!beforeLastGpsInfo.IsValid() || !lastGpsInfo.IsValid())
    return false;

  double const distM =
      ms::DistanceOnEarth(beforeLastGpsInfo.m_latitude, beforeLastGpsInfo.m_longitude,
                          lastGpsInfo.m_latitude, lastGpsInfo.m_longitude);

  double const timeS = lastGpsInfo.m_timestamp - beforeLastGpsInfo.m_timestamp;
  if (timeS <= 0.0)
    return false;

  // |maxDistAfterExtrapolationM| is maximum possible distance from |lastGpsInfo| to
  // the furthest extrapolated point.
  double const maxDistAfterExtrapolationM =
      distM * (Extrapolator::kMaxExtrapolationTimeMs / 1000.0) / timeS;
  // |maxDistForAllExtrapolationsM| is maximum possible distance from |lastGpsInfo| to
  // all extrapolated points in any cases.
  double const maxDistForAllExtrapolationsM =
      kMaxExtrapolationSpeedMPS / kMaxExtrapolationTimeSeconds;
  double const distLastToMeridian180 = ms::DistanceOnEarth(
      lastGpsInfo.m_latitude, lastGpsInfo.m_longitude, lastGpsInfo.m_latitude, 180.0 /* lon2Deg */);
  // Switching off extrapolation if |lastGpsInfo| are so close to meridian 180 that extrapolated
  // points may cross meridian 180 or if |beforeLastGpsInfo| and |lastGpsInfo| are located on
  // different sides of meridian 180.
  if (distLastToMeridian180 < maxDistAfterExtrapolationM ||
      (distLastToMeridian180 < maxDistForAllExtrapolationsM &&
       lastGpsInfo.m_longitude * beforeLastGpsInfo.m_longitude < 0.0) ||
      ms::DistanceOnEarth(lastGpsInfo.m_latitude, lastGpsInfo.m_longitude, 90.0 /* lat2Deg */,
                          lastGpsInfo.m_longitude) < maxDistAfterExtrapolationM ||
      ms::DistanceOnEarth(lastGpsInfo.m_latitude, lastGpsInfo.m_longitude, -90.0 /* lat2Deg */,
                          lastGpsInfo.m_longitude) < maxDistAfterExtrapolationM)
  {
    return false;
  }

  // Note. |timeS| may be less than zero. (beforeLastGpsInfo.m_timestampS >=
  // lastGpsInfo.m_timestampS) It may happen in rare cases because GpsInfo::m_timestampS is not
  // monotonic generally. Please see comment in declaration of class GpsInfo for details.

  // @TODO(bykoianko) Switching off extrapolation based on acceleration should be implemented.
  // Switching off extrapolation based on speed, distance and time.
  return timeS > 0 && distM / timeS <= kMaxExtrapolationSpeedMPS &&
         distM <= kMaxExtrapolationDistMeters && timeS <= kMaxExtrapolationTimeSeconds;
}

// Extrapolator ------------------------------------------------------------------------------------
Extrapolator::Extrapolator(ExtrapolatedLocationUpdateFn const & update)
  : m_isEnabled(false), m_extrapolatedLocationUpdate(update)
{
  RunTaskOnBackgroundThread(false /* delayed */);
}

void Extrapolator::OnLocationUpdate(location::GpsInfo const & gpsInfo)
{
  {
    lock_guard<mutex> guard(m_mutex);
    m_beforeLastGpsInfo = m_lastGpsInfo;
    m_lastGpsInfo = gpsInfo;
    m_extrapolationCounter = 0;
    // Canceling all background tasks which are put to the queue before the task run in this method.
    m_extrapolatedUpdateMinValid = m_extrapolatedUpdateCounter + 1;
  }
  RunTaskOnBackgroundThread(false /* delayed */);
}

void Extrapolator::Enable(bool enabled)
{
  lock_guard<mutex> guard(m_mutex);
  m_isEnabled = enabled;
}

void Extrapolator::ExtrapolatedLocationUpdate(uint64_t extrapolatedUpdateCounter)
{
  location::GpsInfo gpsInfo;
  {
    lock_guard<mutex> guard(m_mutex);
    // Canceling all calls of the method which were activated before |m_extrapolatedUpdateMinValid|.
    if (extrapolatedUpdateCounter < m_extrapolatedUpdateMinValid)
      return;

    uint64_t const extrapolationTimeMs = kExtrapolationPeriodMs * m_extrapolationCounter;
    if (extrapolationTimeMs < kMaxExtrapolationTimeMs && m_lastGpsInfo.IsValid())
    {
      if (DoesExtrapolationWork())
      {
        gpsInfo = LinearExtrapolation(m_beforeLastGpsInfo, m_lastGpsInfo, extrapolationTimeMs);
      }
      else if (m_lastGpsInfo.IsValid())
      {
        gpsInfo = m_lastGpsInfo;
      }
    }
  }

  if (gpsInfo.IsValid())
  {
    GetPlatform().RunTask(Platform::Thread::Gui,
                          [this, gpsInfo]() { m_extrapolatedLocationUpdate(gpsInfo); });
  }

  {
    lock_guard<mutex> guard(m_mutex);
    if (m_extrapolationCounter != kExtrapolationCounterUndefined)
      ++m_extrapolationCounter;
  }

  // Calling ExtrapolatedLocationUpdate() in |kExtrapolationPeriodMs| milliseconds.
  RunTaskOnBackgroundThread(true /* delayed */);
}

void Extrapolator::RunTaskOnBackgroundThread(bool delayed)
{
  uint64_t extrapolatedUpdateCounter = 0;
  {
    lock_guard<mutex> guard(m_mutex);
    ++m_extrapolatedUpdateCounter;
    extrapolatedUpdateCounter = m_extrapolatedUpdateCounter;
  }

  if (delayed)
  {
    auto constexpr kExtrapolationPeriod = std::chrono::milliseconds(kExtrapolationPeriodMs);
    GetPlatform().RunDelayedTask(Platform::Thread::Background, kExtrapolationPeriod,
                                 [this, extrapolatedUpdateCounter] {
                                   ExtrapolatedLocationUpdate(extrapolatedUpdateCounter);
                                 });
    return;
  }

  GetPlatform().RunTask(Platform::Thread::Background, [this, extrapolatedUpdateCounter] {
    ExtrapolatedLocationUpdate(extrapolatedUpdateCounter);
  });
}

bool Extrapolator::DoesExtrapolationWork() const
{
  if (!m_isEnabled || m_extrapolationCounter == kExtrapolationCounterUndefined)
    return false;

  return AreCoordsGoodForExtrapolation(m_beforeLastGpsInfo, m_lastGpsInfo);
}
}  // namespace extrapolation
