#include "map/track_statistics.hpp"

#include "map/elevation_info.hpp"

#include "platform/distance.hpp"
#include "platform/duration.hpp"

void TrackStatistics::CalculateDuration(kml::MultiGeometry const & geometry)
{
  if (!geometry.HasTimestamps())
    return;

  // A track may carry timestamps on some lines only (e.g. GeoJSON import drops a line's
  // timestamps when they are broken, KML drops non-monotonic ones), so skip lines without
  // them instead of requiring every line to have timestamps.
  for (auto const & ts : geometry.m_timestamps)
  {
    if (ts.empty())
      continue;
    ASSERT_GREATER_OR_EQUAL(ts.back(), ts.front(), ());
    m_duration += ts.back() - ts.front();
  }
}

void TrackStatistics::CalculateAltitudes(ElevationInfo const & elevation, geometry::Altitude threshold)
{
  ASSERT(!elevation.IsEmpty(), ());
  auto const info = elevation.CalculateAltitudesInfo(threshold);
  m_ascent = info.GetTotalAscent();
  m_descent = info.GetTotalDescent();
  m_minElevation = info.m_minAltitude;
  m_maxElevation = info.m_maxAltitude;
}

std::string TrackStatistics::GetFormattedLength() const
{
  return platform::Distance::CreateFormatted(m_length).ToString();
}

std::string TrackStatistics::GetFormattedDuration() const
{
  return platform::Duration(static_cast<int>(m_duration)).GetPlatformLocalizedString();
}

std::string TrackStatistics::GetFormattedAscent() const
{
  return platform::Distance::FormatAltitude(m_ascent);
}

std::string TrackStatistics::GetFormattedDescent() const
{
  return platform::Distance::FormatAltitude(m_descent);
}

std::string TrackStatistics::GetFormattedMinElevation() const
{
  return platform::Distance::FormatAltitude(m_minElevation);
}

std::string TrackStatistics::GetFormattedMaxElevation() const
{
  return platform::Distance::FormatAltitude(m_maxElevation);
}
