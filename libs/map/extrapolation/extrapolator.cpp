#include "map/extrapolation/extrapolator.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"

#include <chrono>
#include <memory>
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

bool IsSameFix(location::GpsInfo const & lhs, location::GpsInfo const & rhs)
{
  // Keep this comparison in sync with GpsInfo, including any new measurement metadata.
  return lhs.m_source == rhs.m_source && lhs.m_timestamp == rhs.m_timestamp && lhs.m_latitude == rhs.m_latitude &&
         lhs.m_longitude == rhs.m_longitude && lhs.m_horizontalAccuracy == rhs.m_horizontalAccuracy &&
         lhs.m_altitude == rhs.m_altitude && lhs.m_verticalAccuracy == rhs.m_verticalAccuracy &&
         lhs.m_bearing == rhs.m_bearing && lhs.m_speed == rhs.m_speed && lhs.m_timing == rhs.m_timing;
}

double TimeBetween(location::GpsInfo const & first, location::GpsInfo const & second)
{
  return first.m_timing && second.m_timing
           ? std::chrono::duration<double>(second.m_timing->m_observedAt - first.m_timing->m_observedAt).count()
           : second.m_timestamp - first.m_timestamp;
}
}  // namespace

namespace extrapolation
{
location::GpsInfo LinearExtrapolation(location::GpsInfo const & gpsInfo1, location::GpsInfo const & gpsInfo2,
                                      uint64_t timeAfterPoint2Ms)
{
  if (!gpsInfo1.HasSameClock(gpsInfo2) || TimeBetween(gpsInfo1, gpsInfo2) <= 0)
  {
    ASSERT(false, ("Incorrect gps data"));
    return gpsInfo2;
  }

  auto const timeBetweenPointsMs = gpsInfo1.m_timing
                                     ? std::chrono::duration_cast<std::chrono::milliseconds>(
                                           gpsInfo2.m_timing->m_observedAt - gpsInfo1.m_timing->m_observedAt)
                                           .count()
                                     : static_cast<int64_t>(TimeBetween(gpsInfo1, gpsInfo2) * 1000);
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
  if (!info1.IsValid() || !info2.IsValid() || !info1.HasSameClock(info2))
    return false;

  double const distM = ms::DistanceOnEarth(info1.m_latitude, info1.m_longitude, info2.m_latitude, info2.m_longitude);

  double const timeS = TimeBetween(info1, info2);
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

struct Extrapolator::State
{
  ExtrapolatedLocationUpdateFn m_update;
  NowFn m_now;
  ScheduleFn m_schedule;
  uint64_t m_generation = 0;
  bool m_enabled = false;
  location::GpsInfo m_last;
  location::GpsInfo m_beforeLast;
  Clock::time_point m_lastDistinctUpdate;
};

Extrapolator::Extrapolator(ExtrapolatedLocationUpdateFn const & update)
  : Extrapolator(update, [] { return Clock::now(); }, [](std::chrono::milliseconds delay, Task task)
{
  GetPlatform().RunDelayedTask(Platform::Thread::Background, delay, [task = std::move(task)]() mutable
  { GetPlatform().RunTask(Platform::Thread::Gui, std::move(task)); });
})
{}

Extrapolator::Extrapolator(ExtrapolatedLocationUpdateFn const & update, NowFn now, ScheduleFn schedule)
  : m_state(std::make_shared<State>())
{
  m_state->m_update = update;
  m_state->m_now = std::move(now);
  m_state->m_schedule = std::move(schedule);
}

Extrapolator::~Extrapolator()
{
  ++m_state->m_generation;
}

void Extrapolator::OnLocationUpdate(location::GpsInfo const & gpsInfo)
{
  if (!gpsInfo.IsValid())
  {
    Reset();
    return;
  }

  auto const state = m_state;
  auto const now = state->m_now();
  if (gpsInfo.m_timing && gpsInfo.m_timing->m_observedAt > now)
    return;

  if (state->m_last.IsValid() && !state->m_last.HasSameClock(gpsInfo))
    Reset();

  if (state->m_last.IsValid() && IsSameFix(state->m_last, gpsInfo))
    return;

  double const delta = state->m_last.IsValid() ? TimeBetween(state->m_last, gpsInfo) : 0;
  if (gpsInfo.m_timing && state->m_last.IsValid() && delta < 0)
    return;

  bool const correction = state->m_last.IsValid() && delta == 0;
  if (!correction)
  {
    // UTC can roll back. Start a new baseline instead of extrapolating across the change.
    state->m_beforeLast = delta > 0 ? state->m_last : location::GpsInfo{};
    state->m_lastDistinctUpdate = gpsInfo.m_timing ? gpsInfo.m_timing->m_observedAt : now;
  }
  location::GpsInfo const measurement = gpsInfo;
  state->m_last = measurement;

  auto const generation = ++state->m_generation;
  state->m_update(measurement, UpdateType::Measurement);
  if (state->m_generation == generation)
    Schedule(state, generation);
}

void Extrapolator::Enable(bool enabled)
{
  if (m_state->m_enabled == enabled)
    return;

  auto const generation = ++m_state->m_generation;
  m_state->m_enabled = enabled;
  if (enabled)
    Schedule(m_state, generation);
}

void Extrapolator::Reset()
{
  ++m_state->m_generation;
  m_state->m_last = {};
  m_state->m_beforeLast = {};
}

void Extrapolator::Schedule(std::shared_ptr<State> const & state, uint64_t generation)
{
  if (!state->m_enabled || !AreCoordsGoodForExtrapolation(state->m_beforeLast, state->m_last) ||
      state->m_now() - state->m_lastDistinctUpdate >= std::chrono::milliseconds(kMaxExtrapolationTimeMs))
  {
    return;
  }

  std::weak_ptr<State> const weak = state;
  state->m_schedule(std::chrono::milliseconds(kExtrapolationPeriodMs), [weak, generation]
  {
    if (auto const state = weak.lock())
      Deliver(state, generation);
  });
}

void Extrapolator::Deliver(std::shared_ptr<State> const & state, uint64_t generation)
{
  if (state->m_generation != generation || !state->m_last.IsValid())
    return;

  auto const elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(state->m_now() - state->m_lastDistinctUpdate);
  if (!state->m_enabled || elapsed >= std::chrono::milliseconds(kMaxExtrapolationTimeMs) ||
      !AreCoordsGoodForExtrapolation(state->m_beforeLast, state->m_last))
  {
    return;
  }

  location::GpsInfo const gpsInfo = LinearExtrapolation(state->m_beforeLast, state->m_last, elapsed.count());
  state->m_update(gpsInfo, UpdateType::Prediction);

  if (state->m_generation == generation)
    Schedule(state, generation);
}
}  // namespace extrapolation
