#include "map/track_statistics.hpp"

#include "geometry/mercator.hpp"
#include "base/logging.hpp"

using namespace geometry;
using namespace location;
using namespace kml;

double constexpr kInvalidTimestamp = std::numeric_limits<double>::min();
PointWithAltitude const kInvalidPoint = {m2::PointD::Zero(), kInvalidAltitude};

TrackStatistics::TrackStatistics()
  : m_length(0),
    m_duration(0),
    m_ascent(0),
    m_descent(0),
    m_minElevation(kDefaultAltitudeMeters),
    m_maxElevation(kDefaultAltitudeMeters),
    m_previousPoint(kInvalidPoint),
    m_previousTimestamp(kInvalidTimestamp)
{}

TrackStatistics::TrackStatistics(MultiGeometry const & geometry)
  : TrackStatistics()
{
  for (auto const & line : geometry.m_lines)
    AddPoints(line);
  if (geometry.HasTimestamps())
  {
    for (size_t i = 0; i < geometry.m_timestamps.size(); ++i)
    {
      ASSERT(geometry.HasTimestampsFor(i), ());
      AddTimestamps(geometry.m_timestamps[i]);
    }
  }
}

void TrackStatistics::AddGpsInfoPoint(GpsInfo const & point)
{
  auto const pointWithAltitude = geometry::PointWithAltitude(mercator::FromLatLon(point.m_latitude, point.m_longitude), point.m_altitude);
  auto const altitude = Altitude(point.m_altitude);
  if (HasNoPoints())
  {
    m_minElevation = altitude;
    m_maxElevation = altitude;
    m_previousPoint = pointWithAltitude;
    m_previousTimestamp = point.m_timestamp;
    return;
  }

  m_minElevation = std::min(m_minElevation, altitude);
  m_maxElevation = std::max(m_maxElevation, altitude);

  auto const deltaAltitude = altitude - m_previousPoint.GetAltitude();
  if (deltaAltitude > 0)
    m_ascent += deltaAltitude;
  else
    m_descent -= deltaAltitude;
  m_length += mercator::DistanceOnEarth(m_previousPoint.GetPoint(), pointWithAltitude.GetPoint());
  m_duration += point.m_timestamp - m_previousTimestamp;

  m_previousPoint = pointWithAltitude;
  m_previousTimestamp = point.m_timestamp;
}

void TrackStatistics::AddPoints(Points const & points)
{
  if (points.empty())
    return;

  bool const hasNoPoints = HasNoPoints();
  auto const & firstPoint = points[0];
  auto const altitude = firstPoint.GetAltitude();

  m_minElevation = hasNoPoints ? altitude : std::min(m_minElevation, altitude);
  m_maxElevation = hasNoPoints ? altitude : std::max(m_maxElevation, altitude);
  m_previousPoint = firstPoint;

  for (size_t i = 1; i < points.size(); ++i)
  {
    auto const & point = points[i];
    auto const pointAltitude = point.GetAltitude();

    m_minElevation = std::min(m_minElevation, pointAltitude);
    m_maxElevation = std::max(m_maxElevation, pointAltitude);

    auto const deltaAltitude = pointAltitude - m_previousPoint.GetAltitude();
    if (deltaAltitude > 0)
      m_ascent += deltaAltitude;
    else
      m_descent -= deltaAltitude;
    m_length += mercator::DistanceOnEarth(m_previousPoint.GetPoint(), point.GetPoint());

    m_previousPoint = point;
  }
}

void TrackStatistics::AddTimestamps(Timestamps const & timestamps)
{
  if (timestamps.empty())
    return;
  m_duration += timestamps.back() - timestamps.front();
  m_previousTimestamp = timestamps.back();
}

bool TrackStatistics::HasNoPoints() const
{
  return m_previousPoint == kInvalidPoint;
}
