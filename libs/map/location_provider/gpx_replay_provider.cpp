#include "map/location_provider/gpx_replay_provider.hpp"

#ifdef DEBUG

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>
#include <chrono>

namespace location_provider
{
GpxReplayProvider & GpxReplayProvider::Instance()
{
  static GpxReplayProvider instance;
  return instance;
}

void GpxReplayProvider::Arm(kml::TrackData const & track, std::optional<double> accuracyOverride)
{
  CHECK(!track.m_geometry.m_lines.empty(), ("GpxReplayProvider::Arm() requires a track with at least one line"));
  m_line = track.m_geometry.m_lines[0];
  CHECK(!m_line.empty(), ("GpxReplayProvider::Arm() requires a non-empty line"));

  m_cumulativeDistanceMeters.assign(m_line.size(), 0.0);
  for (size_t i = 1; i < m_line.size(); ++i)
  {
    double const segmentLength = mercator::DistanceOnEarth(m_line[i - 1].GetPoint(), m_line[i].GetPoint());
    m_cumulativeDistanceMeters[i] = m_cumulativeDistanceMeters[i - 1] + segmentLength;
  }

  m_accuracyOverride = accuracyOverride;
  m_armTime = std::chrono::steady_clock::now();
  m_finished = false;
  m_armed = true;
  m_startDelaySeconds = kStartDelaySeconds;
  m_speedMetersPerSecond = kDefaultSpeedMetersPerSecond;
}

void GpxReplayProvider::Disarm()
{
  m_armed = false;
}

void GpxReplayProvider::SetTimingForTest(double startDelaySeconds, double speedMetersPerSecond)
{
  m_startDelaySeconds = startDelaySeconds;
  m_speedMetersPerSecond = speedMetersPerSecond;
}

std::optional<location::GpsInfo> GpxReplayProvider::GetCurrentReading()
{
  if (!m_armed || m_finished)
    return std::nullopt;

  double const elapsedSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - m_armTime).count();
  double const totalDistanceMeters = m_cumulativeDistanceMeters.back();

  double distanceAlongMeters = 0.0;
  if (elapsedSeconds > m_startDelaySeconds)
    distanceAlongMeters = (elapsedSeconds - m_startDelaySeconds) * m_speedMetersPerSecond;

  if (distanceAlongMeters >= totalDistanceMeters)
  {
    distanceAlongMeters = totalDistanceMeters;
    m_finished = true;
  }

  auto const reading = SynthesizeGpsInfoAtDistance(distanceAlongMeters);
  LOG(LINFO, ("GpxReplayProvider tick", distanceAlongMeters, "/", totalDistanceMeters, reading.m_latitude, reading.m_longitude));
  return reading;
}

location::GpsInfo GpxReplayProvider::SynthesizeGpsInfoAtDistance(double distanceMeters) const
{
  location::GpsInfo info;
  info.m_source = location::EPredictor;

  ms::LatLon latLon;
  if (m_line.size() == 1)
  {
    latLon = m_line[0].ToLatLon();
  }
  else
  {
    size_t i = 0;
    while (i + 2 < m_line.size() && m_cumulativeDistanceMeters[i + 1] < distanceMeters)
      ++i;

    double const segStart = m_cumulativeDistanceMeters[i];
    double const segEnd = m_cumulativeDistanceMeters[i + 1];
    double const segLen = segEnd - segStart;
    double const t = segLen > 0.0 ? std::clamp((distanceMeters - segStart) / segLen, 0.0, 1.0) : 0.0;

    auto const & a = m_line[i].GetPoint();
    auto const & b = m_line[i + 1].GetPoint();
    latLon = mercator::ToLatLon(a + (b - a) * t);
  }

  info.m_latitude = latLon.m_lat;
  info.m_longitude = latLon.m_lon;
  info.m_horizontalAccuracy = m_accuracyOverride.value_or(kDefaultAccuracyMeters);
  info.m_timestamp = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();

  return info;
}
}  // namespace location_provider

#endif  // DEBUG
