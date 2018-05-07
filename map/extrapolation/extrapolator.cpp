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
double constexpr kMaxExtrapolationSpeedMPerS = 120.0;

double LinearExtrapolationOfOneParam(double param1, double param2, uint64_t timeBetweenPointsMs,
                                     uint64_t timeAfterPoint2Ms)
{
  return (param2 - param1) * timeAfterPoint2Ms / timeBetweenPointsMs;
}
}  // namespace

namespace position_extrapolator
{
using namespace std;

location::GpsInfo LinearExtrapolation(location::GpsInfo const & point1, location::GpsInfo const & point2,
                                      uint64_t timeAfterPoint2Ms)
{
  auto const timeBetweenPointsMs =
      static_cast<uint64_t>((point2.m_timestamp - point1.m_timestamp) * 1000);

  location::GpsInfo extrapolated = point2;

  extrapolated.m_timestamp += timeAfterPoint2Ms;

  extrapolated.m_longitude += LinearExtrapolationOfOneParam(
      point1.m_longitude, point2.m_longitude, timeBetweenPointsMs, timeAfterPoint2Ms);

  extrapolated.m_latitude += LinearExtrapolationOfOneParam(point1.m_latitude, point2.m_latitude,
                                                           timeBetweenPointsMs, timeAfterPoint2Ms);

  extrapolated.m_horizontalAccuracy +=
      LinearExtrapolationOfOneParam(point1.m_horizontalAccuracy, point2.m_horizontalAccuracy,
                                    timeBetweenPointsMs, timeAfterPoint2Ms);
  extrapolated.m_altitude += LinearExtrapolationOfOneParam(point1.m_altitude, point2.m_altitude,
                                                           timeBetweenPointsMs, timeAfterPoint2Ms);

  if (point1.m_verticalAccuracy != -1 && point2.m_verticalAccuracy != -1)
  {
    extrapolated.m_verticalAccuracy +=
        LinearExtrapolationOfOneParam(point1.m_verticalAccuracy, point2.m_verticalAccuracy,
                                      timeBetweenPointsMs, timeAfterPoint2Ms);
  }

  if (point1.m_bearing != -1 && point2.m_bearing != -1)
  {
    extrapolated.m_bearing += LinearExtrapolationOfOneParam(point1.m_bearing, point2.m_bearing,
                                                            timeBetweenPointsMs, timeAfterPoint2Ms);
  }

  if (point1.m_speed != -1 && point2.m_speed != -1)
  {
    extrapolated.m_speed += LinearExtrapolationOfOneParam(point1.m_speed, point2.m_speed,
                                                          timeBetweenPointsMs, timeAfterPoint2Ms);
  }
  return extrapolated;
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
          if (m_lastGpsInfo.m_source != location::EUndefine)
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
      m_lastGpsInfo.m_source == location::EUndefine ||
      m_beforeLastGpsInfo.m_source == location::EUndefine ||
      m_beforeLastGpsInfo.m_timestamp >= m_lastGpsInfo.m_timestamp)
  {
    return false;
  }

  double const distM =
      ms::DistanceOnEarth(m_beforeLastGpsInfo.m_latitude, m_beforeLastGpsInfo.m_longitude,
                          m_lastGpsInfo.m_latitude, m_lastGpsInfo.m_longitude);
  double const timeS = m_lastGpsInfo.m_timestamp - m_beforeLastGpsInfo.m_timestamp;

  // Switching off  extrapolation based on speed.
  return distM / timeS < kMaxExtrapolationSpeedMPerS;
  // @TODO(bykoianko) Switching off extrapolation based on acceleration should be implemented.
}

// Extrapolator ------------------------------------------------------------------------------------
Extrapolator::Extrapolator(ExtrapolatedLocationUpdate const & update)
  : m_extrapolatedLocationThread()
{
  m_extrapolatedLocationThread.Create(make_unique<Routine>(update));
}

void Extrapolator::OnLocationUpdate(location::GpsInfo & info)
{
  auto * routine = m_extrapolatedLocationThread.GetRoutineAs<Routine>();
  CHECK(routine, ());
  routine->SetGpsInfo(info);
}
}  // namespace position_extrapolator
