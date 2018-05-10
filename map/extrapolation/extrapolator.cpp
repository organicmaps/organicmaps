#include "map/extrapolation/extrapolator.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include <chrono>
#include <memory>

namespace
{
uint64_t constexpr kMaxExtrapolationTimeMs = 1000;
uint64_t constexpr kExtrapolationPeriodMs = 200;
double constexpr kMaxExtrapolationSpeedMPS = 120.0;

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
  // taking into account that they are periodic.

  if (gpsInfo1.HasSpeed() && gpsInfo2.HasSpeed())
    result.m_speed = e.Extrapolate(gpsInfo1.m_speed, gpsInfo2.m_speed);

  return result;
}

// Extrapolator::Routine ---------------------------------------------------------------------------
Extrapolator::Routine::Routine(ExtrapolatedLocationUpdate const & update)
  : m_extrapolatedLocationUpdate(update)
{
}

void Extrapolator::Routine::Do()
{
  while (!IsCancelled())
  {
    {
      GetPlatform().RunTask(Platform::Thread::Gui, [this]() {
        lock_guard<mutex> guard(m_mutex);
        uint64_t const extrapolationTimeMs = kExtrapolationPeriodMs * m_extrapolationCounter;
        if (extrapolationTimeMs >= kMaxExtrapolationTimeMs)
          return;

        if (DoesExtrapolationWork(extrapolationTimeMs))
        {
          location::GpsInfo gpsInfo =
              LinearExtrapolation(m_beforeLastGpsInfo, m_lastGpsInfo, extrapolationTimeMs);
          m_extrapolatedLocationUpdate(gpsInfo);
        }
        else
        {
          if (m_lastGpsInfo.m_source != location::EUndefined)
          {
            location::GpsInfo gpsInfo = m_lastGpsInfo;
            m_extrapolatedLocationUpdate(gpsInfo);
          }
        }
      });

      lock_guard<mutex> guard(m_mutex);
      if (m_extrapolationCounter != m_extrapolationCounterUndefined)
        ++m_extrapolationCounter;
    }
    // @TODO(bykoinako) Method m_extrapolatedLocationUpdate() is run on gui thread every
    // |kExtrapolationPeriodMs| milliseconds. But after changing GPS position
    // (that means after a call of method Routine::SetGpsInfo())
    // m_extrapolatedLocationUpdate() should be run immediately on gui thread.
    this_thread::sleep_for(std::chrono::milliseconds(kExtrapolationPeriodMs));
  }
}

void Extrapolator::Routine::SetGpsInfo(location::GpsInfo const & gpsInfo)
{
  lock_guard<mutex> guard(m_mutex);
  m_beforeLastGpsInfo = m_lastGpsInfo;
  m_lastGpsInfo = gpsInfo;
  m_extrapolationCounter = 0;
}

bool Extrapolator::Routine::DoesExtrapolationWork(uint64_t extrapolationTimeMs) const
{
  // Note. It's possible that m_beforeLastGpsInfo.m_timestamp >= m_lastGpsInfo.m_timestamp.
  // It may happen in rare cases because GpsInfo::m_timestamp is not monotonic generally.
  // Please see comment in declaration of class GpsInfo for details.

  if (m_extrapolationCounter == m_extrapolationCounterUndefined ||
      m_lastGpsInfo.m_source == location::EUndefined ||
      m_beforeLastGpsInfo.m_source == location::EUndefined ||
      m_beforeLastGpsInfo.m_timestamp >= m_lastGpsInfo.m_timestamp)
  {
    return false;
  }

  double const distM =
      ms::DistanceOnEarth(m_beforeLastGpsInfo.m_latitude, m_beforeLastGpsInfo.m_longitude,
                          m_lastGpsInfo.m_latitude, m_lastGpsInfo.m_longitude);
  double const timeS = m_lastGpsInfo.m_timestamp - m_beforeLastGpsInfo.m_timestamp;

  // Switching off extrapolation based on speed.
  return distM / timeS < kMaxExtrapolationSpeedMPS;
  // @TODO(bykoianko) Switching off extrapolation based on acceleration should be implemented.
}

// Extrapolator ------------------------------------------------------------------------------------
Extrapolator::Extrapolator(ExtrapolatedLocationUpdate const & update)
  : m_extrapolatedLocationThread()
{
  m_extrapolatedLocationThread.Create(make_unique<Routine>(update));
}

void Extrapolator::OnLocationUpdate(location::GpsInfo const & info)
{
  auto * routine = m_extrapolatedLocationThread.GetRoutineAs<Routine>();
  CHECK(routine, ());
  routine->SetGpsInfo(info);
}
}  // namespace extrapolation
