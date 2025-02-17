#include "map/track_statistics.hpp"

#include "geometry/mercator.hpp"
#include "base/logging.hpp"

using namespace geometry;

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

TrackStatistics::TrackStatistics(kml::MultiGeometry const & geometry)
  : TrackStatistics()
{
  for (auto const & line : geometry.m_lines)
    AddPoints(line, true);
  if (geometry.HasTimestamps())
  {
    for (size_t i = 0; i < geometry.m_timestamps.size(); ++i)
    {
      ASSERT(geometry.HasTimestampsFor(i), ());
      AddTimestamps(geometry.m_timestamps[i], true);
    }
  }
}

void TrackStatistics::AddGpsInfoPoint(location::GpsInfo const & point)
{
  auto const pointWithAltitude = geometry::PointWithAltitude(mercator::FromLatLon(point.m_latitude, point.m_longitude), point.m_altitude);

  AddPoints({pointWithAltitude}, false);
  AddTimestamps({point.m_timestamp}, false);
}

void TrackStatistics::AddPoints(kml::MultiGeometry::LineT const & line, bool isNewSegment)
{
  if (line.empty())
    return;

  size_t startIndex = 0;
  if (HasNoPoints() || isNewSegment)
  {
    InitializeNewSegment(line[0]);
    startIndex = 1;
  }

  ProcessPoints(line, startIndex);
}

void TrackStatistics::InitializeNewSegment(PointWithAltitude const & firstPoint)
{
  auto const altitude = firstPoint.GetAltitude();

  bool const hasNoPoints = HasNoPoints();
  m_minElevation = hasNoPoints ? altitude : std::min(m_minElevation, altitude);
  m_maxElevation = hasNoPoints ? altitude : std::max(m_maxElevation, altitude);

  m_previousPoint = firstPoint;
}

void TrackStatistics::ProcessPoints(kml::MultiGeometry::LineT const & points, size_t startIndex)
{
  for (size_t i = startIndex; i < points.size(); ++i)
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

void TrackStatistics::AddTimestamps(kml::MultiGeometry::TimeT const & timestamps, bool isNewSegment)
{
  if (timestamps.empty())
    return;

  if (m_previousTimestamp == kInvalidTimestamp)
    m_previousTimestamp = timestamps.front();

  auto const baseTimestamp = isNewSegment ? timestamps.front() : m_previousTimestamp;
  m_duration += timestamps.back() - baseTimestamp;
  m_previousTimestamp = timestamps.back();
}

bool TrackStatistics::HasNoPoints() const
{
  return m_previousPoint == kInvalidPoint;
}
