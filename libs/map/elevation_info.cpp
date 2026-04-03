#include "map/elevation_info.hpp"

#include "map/chart_generator.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"
#include "geometry/simplification.hpp"

#include "indexer/map_style_reader.hpp"

#include "base/math.hpp"

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

void ElevationInfo::Assign(std::vector<double> const & segDistances, geometry::Altitudes const & altitudes)
{
  ASSERT_EQUAL(segDistances.size() + 1, altitudes.size(), ());

  Points pts;
  pts.reserve(altitudes.size());
  pts.emplace_back(0, altitudes[0]);
  for (size_t i = 0; i < segDistances.size(); ++i)
    pts.emplace_back(segDistances[i], altitudes[i + 1]);

  m_lines.clear();
  m_lines.push_back(std::move(pts));
  m_difficulty = Difficulty::Unknown;
}

void ElevationInfo::Simplify(double altitudeDeviation)
{
  class IterT
  {
    Points const & m_points;
    size_t m_ind = 0;

  public:
    IterT(Points const & points, bool isBeg) : m_points(points) { m_ind = isBeg ? 0 : m_points.size(); }

    IterT(IterT const & rhs) = default;
    IterT & operator=(IterT const & rhs)
    {
      m_ind = rhs.m_ind;
      return *this;
    }

    bool operator!=(IterT const & rhs) const { return m_ind != rhs.m_ind; }

    IterT & operator++()
    {
      ++m_ind;
      return *this;
    }
    IterT operator+(size_t inc) const
    {
      IterT res = *this;
      res.m_ind += inc;
      return res;
    }
    int64_t operator-(IterT const & rhs) const { return int64_t(m_ind) - int64_t(rhs.m_ind); }

    m2::PointD operator*() const { return {m_points[m_ind].m_distance, double(m_points[m_ind].m_altitude)}; }
  };

  double const squareEps = math::Pow2(altitudeDeviation);
  for (auto & line : m_lines)
  {
    std::vector<m2::PointD> out;
    SimplifyDefault(IterT(line, true), IterT(line, false), squareEps, out);

    size_t const count = out.size();
    line.resize(count);
    for (size_t i = 0; i < count; ++i)
    {
      line[i].m_distance = out[i].x;
      line[i].m_altitude = geometry::Altitude(out[i].y);
    }
  }
}

void ElevationInfo::CalculateAscentDescent(uint32_t & totalAscentM, uint32_t & totalDescentM,
                                           geometry::Altitude threshold) const
{
  ASSERT(threshold >= 0, ());

  totalAscentM = 0;
  totalDescentM = 0;
  for (auto const & line : m_lines)
  {
    ASSERT(!line.empty(), ());

    geometry::Altitude refAlt = line[0].m_altitude;
    for (size_t i = 1; i < line.size(); ++i)
    {
      auto const delta = line[i].m_altitude - refAlt;
      if (delta > threshold)
      {
        totalAscentM += delta;
        refAlt = line[i].m_altitude;
      }
      else if (delta < -threshold)
      {
        totalDescentM += -delta;
        refAlt = line[i].m_altitude;
      }
    }
  }
}

bool ElevationInfo::GenerateRouteAltitudeChart(uint32_t width, uint32_t height,
                                               std::vector<uint8_t> & imageRGBAData) const
{
  std::vector<double> distances;
  geometry::Altitudes altitudes;

  double cumulativeOffset = 0;
  for (auto const & line : m_lines)
  {
    for (auto const & point : line)
    {
      distances.push_back(cumulativeOffset + point.m_distance);
      altitudes.push_back(point.m_altitude);
    }

    if (!line.empty())
      cumulativeOffset += line.back().m_distance;
  }

  if (distances.empty())
    return false;

  return maps::GenerateChart(width, height, distances, altitudes, GetStyleReader().GetCurrentStyle(), imageRGBAData);
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
