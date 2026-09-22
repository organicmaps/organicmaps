#include "map/extrapolation/extrapolator.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"
#include "std/target_os.hpp"

#include <chrono>
#include <cmath>
#include <memory>
#if defined(OMIM_OS_ANDROID)
#include <time.h>
#endif

namespace
{
double MonotonicSeconds()
{
#if defined(OMIM_OS_ANDROID)
  // Match elapsedRealtimeNanos(), including suspend time; old fixes must expire during STR.
  timespec time{};
  int const result = clock_gettime(CLOCK_BOOTTIME, &time);
  CHECK_EQUAL(result, 0, ());
  return static_cast<double>(time.tv_sec) + static_cast<double>(time.tv_nsec) / 1e9;
#else
  return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
}
// If the difference of values between two instances of GpsInfo is greater
// than the appropriate constant below the extrapolator will be switched off for
// these two instances of GpsInfo.
double constexpr kMaxExtrapolationSpeedMPS = 75.0;
double constexpr kMaxExtrapolationDistMeters = 100.0;
double constexpr kMaxExtrapolationTimeSeconds = 2.1;

class LinearExtrapolator
{
public:
  LinearExtrapolator(uint64_t timeBetweenMs, uint64_t timeAfterMs)
    : m_timeBetweenMs(timeBetweenMs)
    , m_timeAfterMs(timeAfterMs)
  {
    CHECK_NOT_EQUAL(m_timeBetweenMs, 0, ());
  }

  double Extrapolate(double x1, double x2) const { return x2 + ((x2 - x1) / m_timeBetweenMs) * m_timeAfterMs; }

private:
  uint64_t m_timeBetweenMs;
  uint64_t m_timeAfterMs;
};
}  // namespace

namespace extrapolation
{
location::GpsInfo LinearExtrapolation(location::GpsInfo const & gpsInfo1, location::GpsInfo const & gpsInfo2,
                                      uint64_t timeAfterPoint2Ms)
{
  if (gpsInfo2.m_timestamp <= gpsInfo1.m_timestamp)
  {
    ASSERT(false, ("Incorrect gps data"));
    return gpsInfo2;
  }

  auto const timeBetweenPointsMs = static_cast<uint64_t>((gpsInfo2.m_timestamp - gpsInfo1.m_timestamp) * 1000);
  if (timeBetweenPointsMs == 0)
    return gpsInfo2;

  location::GpsInfo result = gpsInfo2;
  LinearExtrapolator const e(timeBetweenPointsMs, timeAfterPoint2Ms);

  result.m_timestamp += static_cast<double>(timeAfterPoint2Ms) / 1000.0;
  result.m_longitude = math::Clamp(e.Extrapolate(gpsInfo1.m_longitude, gpsInfo2.m_longitude), -180.0, 180.0);
  result.m_latitude = math::Clamp(e.Extrapolate(gpsInfo1.m_latitude, gpsInfo2.m_latitude), -90.0, 90.0);
  result.m_altitude = e.Extrapolate(gpsInfo1.m_altitude, gpsInfo2.m_altitude);

  // @TODO(bykoianko) Now |result.m_bearing| == |gpsInfo2.m_bearing|.
  // In case of |gpsInfo1.HasBearing() && gpsInfo2.HasBearing() == true|
  // consider finding an average value between |gpsInfo1.m_bearing| and |gpsInfo2.m_bearing|
  // taking into account that they are periodic. It's important to implement it
  // because current implementation leads to changing course by steps. It doesn't
  // look nice when the road changes its direction.

  if (gpsInfo1.HasSpeed() && gpsInfo2.HasSpeed())
    result.m_speed = e.Extrapolate(gpsInfo1.m_speed, gpsInfo2.m_speed);

  return result;
}

bool AreCoordsGoodForExtrapolation(location::GpsInfo const & info1, location::GpsInfo const & info2)
{
  if (!info1.IsValid() || !info2.IsValid())
    return false;

  double const distM = ms::DistanceOnEarth(info1.m_latitude, info1.m_longitude, info2.m_latitude, info2.m_longitude);

  double const timeS = info2.m_timestamp - info1.m_timestamp;
  if (timeS <= 0.0)
    return false;

  // |maxDistAfterExtrapolationM| is maximum possible distance from |info2| to
  // the furthest extrapolated point.
  double const maxDistAfterExtrapolationM = distM * (Extrapolator::kMaxExtrapolationTimeMs / 1000.0) / timeS;
  // |maxDistForAllExtrapolationsM| is maximum possible distance from |info2| to
  // all extrapolated points in any cases.
  double const maxDistForAllExtrapolationsM = kMaxExtrapolationSpeedMPS * kMaxExtrapolationTimeSeconds;
  double const distLastGpsInfoToMeridian180 =
      ms::DistanceOnEarth(info2.m_latitude, info2.m_longitude, info2.m_latitude, 180.0 /* lon2Deg */);
  // Switching off extrapolation if |info2| are so close to meridian 180 that extrapolated
  // points may cross meridian 180 or if |info1| and |info2| are located on
  // different sides of meridian 180.
  if (distLastGpsInfoToMeridian180 < maxDistAfterExtrapolationM ||
      (distLastGpsInfoToMeridian180 < maxDistForAllExtrapolationsM && info2.m_longitude * info1.m_longitude < 0.0) ||
      ms::DistanceOnEarth(info2.m_latitude, info2.m_longitude, 90.0 /* lat2Deg */, info2.m_longitude) <
          maxDistAfterExtrapolationM ||
      ms::DistanceOnEarth(info2.m_latitude, info2.m_longitude, -90.0 /* lat2Deg */, info2.m_longitude) <
          maxDistAfterExtrapolationM)
  {
    return false;
  }

  // Note. |timeS| may be less than zero. (info1.m_timestampS >=
  // info2.m_timestampS) It may happen in rare cases because GpsInfo::m_timestampS is not
  // monotonic generally. Please see comment in declaration of class GpsInfo for details.

  // @TODO(bykoianko) Switching off extrapolation based on acceleration should be implemented.
  // Switching off extrapolation based on speed, distance and time.
  return distM / timeS <= kMaxExtrapolationSpeedMPS && distM <= kMaxExtrapolationDistMeters &&
         timeS <= kMaxExtrapolationTimeSeconds;
}

// Extrapolator ------------------------------------------------------------------------------------
Extrapolator::Extrapolator(ExtrapolatedLocationUpdateFn const & update)
  : m_isEnabled(false)
  , m_extrapolatedLocationUpdate(update)
{
  RunTaskOnBackgroundThread(false /* delayed */, 0);
}

void Extrapolator::OnLocationUpdate(location::GpsInfo const & gpsInfo, double ageSeconds)
{
  uint64_t generation;
  {
    std::lock_guard<std::mutex> guard(m_mutex);
    m_beforeLastGpsInfo = m_lastGpsInfo;
    m_lastGpsInfo = gpsInfo;
    m_lastOutput = gpsInfo;
    double const now = MonotonicSeconds();
    bool const validAge = std::isfinite(ageSeconds) && ageSeconds >= 0.0;
    m_lastGpsTime = now - (validAge ? ageSeconds : 0.0);
    m_vehicleMotion.SetFix(validAge ? gpsInfo : location::GpsInfo{}, m_lastGpsTime, now);
    ++m_motionRevision;
    m_consecutiveRuns = 0;
    // Canceling all background tasks which are put to the queue before the task run in this method.
    ++m_locationUpdateCounter;
    m_locationUpdateMinValid = m_locationUpdateCounter;
    generation = m_locationUpdateCounter;
  }
  RunTaskOnBackgroundThread(false /* delayed */, generation);
}

void Extrapolator::OnVehicleSpeed(double speedMps, double ageSeconds, bool valid)
{
  std::lock_guard<std::mutex> guard(m_mutex);
  ++m_motionRevision;
  if (!valid || !std::isfinite(ageSeconds) || ageSeconds < 0.0 || ageSeconds > VehicleMotion::kMaxAgeSeconds)
  {
    m_vehicleMotion.InvalidateSpeed();
    return;
  }
  double const now = MonotonicSeconds();
  m_vehicleMotion.SetSpeed(speedMps, now - ageSeconds, now);
}

void Extrapolator::Enable(bool enabled)
{
  std::lock_guard<std::mutex> guard(m_mutex);
  m_isEnabled = enabled;
}

void Extrapolator::ExtrapolatedLocationUpdate(uint64_t locationUpdateCounter)
{
  location::GpsInfo gpsInfo;
  bool predicted = false;
  uint64_t revision;
  {
    std::lock_guard<std::mutex> guard(m_mutex);
    // Canceling all calls of the method which were activated before |m_locationUpdateMinValid|.
    if (locationUpdateCounter < m_locationUpdateMinValid)
      return;

    revision = m_motionRevision;
    double const now = MonotonicSeconds();
    double const elapsed = now - m_lastGpsTime;
    bool const withinHorizon = elapsed >= 0.0 && elapsed < kMaxExtrapolationTimeMs / 1000.0;
    if (m_lastGpsInfo.IsValid() && (m_consecutiveRuns == 0 || withinHorizon))
    {
      auto vehicle = m_vehicleMotion.Predict(now);
      if (vehicle)
      {
        gpsInfo = *vehicle;
        predicted = true;
      }
      else if (m_vehicleMotion.ShouldHoldPosition())
      {
        gpsInfo = m_lastOutput;
        gpsInfo.m_speed = -1.0;  // Invalid vehicle speed must not continue advancing a prediction.
        predicted = true;
      }
      else if (withinHorizon && DoesExtrapolationWork())
      {
        gpsInfo = LinearExtrapolation(m_beforeLastGpsInfo, m_lastGpsInfo,
                                      m_consecutiveRuns == 0 ? 0 : static_cast<uint64_t>(elapsed * 1000.0));
        predicted = m_consecutiveRuns != 0;
      }
      else
        gpsInfo = m_lastGpsInfo;
      m_lastOutput = gpsInfo;
    }
  }

  if (gpsInfo.IsValid())
    GetPlatform().RunTask(Platform::Thread::Gui, [this, gpsInfo, locationUpdateCounter, revision, predicted]()
    {
      {
        std::lock_guard<std::mutex> guard(m_mutex);
        if (locationUpdateCounter != m_locationUpdateCounter || (predicted && revision != m_motionRevision))
          return;
      }
      m_extrapolatedLocationUpdate(gpsInfo);
    });

  {
    std::lock_guard<std::mutex> guard(m_mutex);
    if (locationUpdateCounter != m_locationUpdateCounter)
      return;
    if (m_consecutiveRuns != kExtrapolationCounterUndefined)
      ++m_consecutiveRuns;
  }

  // Calling ExtrapolatedLocationUpdate() in |kExtrapolationPeriodMs| milliseconds.
  RunTaskOnBackgroundThread(true /* delayed */, locationUpdateCounter);
}

void Extrapolator::RunTaskOnBackgroundThread(bool delayed, uint64_t locationUpdateCounter)
{
  if (delayed)
  {
    auto constexpr period = std::chrono::milliseconds(kExtrapolationPeriodMs);
    GetPlatform().RunDelayedTask(Platform::Thread::Background, period,
                                 [this, locationUpdateCounter] { ExtrapolatedLocationUpdate(locationUpdateCounter); });
  }
  else
  {
    GetPlatform().RunTask(Platform::Thread::Background,
                          [this, locationUpdateCounter] { ExtrapolatedLocationUpdate(locationUpdateCounter); });
  }
}

bool Extrapolator::DoesExtrapolationWork() const
{
  if (!m_isEnabled || m_consecutiveRuns == kExtrapolationCounterUndefined)
    return false;

  return AreCoordsGoodForExtrapolation(m_beforeLastGpsInfo, m_lastGpsInfo);
}
}  // namespace extrapolation
