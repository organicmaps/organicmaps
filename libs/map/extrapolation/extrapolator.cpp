#include "map/extrapolation/extrapolator.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"

#include <chrono>
#include <utility>

namespace
{
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

Extrapolator::Extrapolator(LocationUpdateFn update) : m_update(std::move(update)) {}

void Extrapolator::OnLocationUpdate(location::GpsInfo const & gpsInfo)
{
  uint64_t generation;
  {
    std::lock_guard guard(m_mutex);
    m_beforeLastGpsInfo = m_lastGpsInfo;
    m_lastGpsInfo = gpsInfo;
    m_consecutiveRuns = 0;
    generation = ++m_generation;
  }
  RunTaskOnBackgroundThread(false /* delayed */, generation);
}

void Extrapolator::Enable(bool enabled)
{
  std::lock_guard guard(m_mutex);
  m_isEnabled = enabled;
}

void Extrapolator::ProcessLocationTick(uint64_t generation)
{
  location::GpsInfo gpsInfo;
  UpdateType type;
  bool hasNext;
  {
    std::lock_guard guard(m_mutex);
    if (generation != m_generation || !m_lastGpsInfo.IsValid())
      return;

    type = m_consecutiveRuns == 0 ? UpdateType::Measurement : UpdateType::Prediction;
    uint64_t const predictionTimeMs = kExtrapolationPeriodMs * m_consecutiveRuns;
    if (type == UpdateType::Prediction && m_isEnabled &&
        AreCoordsGoodForExtrapolation(m_beforeLastGpsInfo, m_lastGpsInfo))
      gpsInfo = LinearExtrapolation(m_beforeLastGpsInfo, m_lastGpsInfo, predictionTimeMs);
    else
      gpsInfo = m_lastGpsInfo;

    hasNext = kExtrapolationPeriodMs * ++m_consecutiveRuns < kMaxExtrapolationTimeMs;
  }

  GetPlatform().RunTask(Platform::Thread::Gui, [this, gpsInfo, type] { m_update(gpsInfo, type); });

  if (hasNext)
    RunTaskOnBackgroundThread(true /* delayed */, generation);
}

void Extrapolator::RunTaskOnBackgroundThread(bool delayed, uint64_t generation)
{
  auto task = [this, generation] { ProcessLocationTick(generation); };
  if (delayed)
    GetPlatform().RunDelayedTask(Platform::Thread::Background, std::chrono::milliseconds(kExtrapolationPeriodMs),
                                 std::move(task));
  else
    GetPlatform().RunTask(Platform::Thread::Background, std::move(task));
}
}  // namespace extrapolation
