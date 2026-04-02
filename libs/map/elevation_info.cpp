#include "map/elevation_info.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"

ElevationInfo::ElevationInfo(std::vector<GeometryLine> const & lines)
{
  for (auto const & line : lines)
  {
    // Enclosing Track class should avoid empty geometries.
    ASSERT(!line.empty(), ());

    Points pts;
    pts.reserve(line.size());

    double distance = 0;
    pts.emplace_back(distance, line[0].GetAltitude());
    for (size_t i = 1; i < line.size(); ++i)
    {
      distance += mercator::DistanceOnEarth(line[i - 1].GetPoint(), line[i].GetPoint());
      pts.emplace_back(distance, line[i].GetAltitude());
    }

    m_lines.push_back(std::move(pts));
  }

  /// @todo(KK) Implement difficulty calculation.
  m_difficulty = Difficulty::Unknown;
}

void GpsTrackElevation::AddGpsPoints(GpsPoints const & points)
{
  if (points.empty())
    return;

  // GPS track recording always produces a single line.
  if (m_lines.empty())
    m_lines.emplace_back();

  auto & line = m_lines.back();

  for (auto const & gps : points)
  {
    ms::LatLon const ll(gps.m_latitude, gps.m_longitude);

    if (m_hasLastPoint)
      m_lastDistance += ms::DistanceOnEarth(m_lastLatLon, ll);

    line.emplace_back(m_lastDistance, static_cast<geometry::Altitude>(gps.m_altitude));
    m_lastLatLon = ll;
    m_hasLastPoint = true;
  }
}

void GpsTrackElevation::Clear()
{
  m_lines.clear();
  m_difficulty = Difficulty::Unknown;
  m_lastDistance = 0;
  m_hasLastPoint = false;
}

size_t GpsTrackElevation::GetSize() const
{
  size_t size = 0;
  for (auto const & line : m_lines)
    size += line.size();
  return size;
}
