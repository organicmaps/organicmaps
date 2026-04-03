#include "map/track_statistics.hpp"

#include "geometry/mercator.hpp"

#include "platform/distance.hpp"
#include "platform/duration.hpp"

void TrackStatistics::CalculateDuration(kml::MultiGeometry const & geometry)
{
  if (!geometry.HasTimestamps())
    return;

  for (size_t i = 0; i < geometry.m_timestamps.size(); ++i)
  {
    ASSERT(geometry.HasTimestampsFor(i), ());
    auto const & ts = geometry.m_timestamps[i];
    if (!ts.empty())
    {
      ASSERT_GREATER_OR_EQUAL(ts.back(), ts.front(), ());
      m_duration += ts.back() - ts.front();
    }
  }
}

void TrackStatistics::AddGpsInfoPoint(location::GpsInfo const & point)
{
  ASSERT_GREATER_OR_EQUAL(point.m_timestamp, 0, ());
  auto const pt = mercator::FromLatLon(point.m_latitude, point.m_longitude);

  if (m_previousTimestamp >= 0)
  {
    m_length += mercator::DistanceOnEarth(m_previousPoint, pt);
    ASSERT_GREATER_OR_EQUAL(point.m_timestamp, m_previousTimestamp, ());
    m_duration += point.m_timestamp - m_previousTimestamp;
  }

  if (point.HasAltitude())
  {
    auto const altitude = geometry::Altitude(point.m_altitude);

    if (m_previousAltitude != geometry::kInvalidAltitude)
    {
      m_minElevation = std::min(m_minElevation, altitude);
      m_maxElevation = std::max(m_maxElevation, altitude);

      auto const delta = altitude - m_previousAltitude;
      if (delta > 0)
        m_ascent += delta;
      else
        m_descent -= delta;
    }
    else
    {
      m_minElevation = altitude;
      m_maxElevation = altitude;
    }

    m_previousAltitude = altitude;
  }

  m_previousPoint = pt;
  m_previousTimestamp = point.m_timestamp;
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
