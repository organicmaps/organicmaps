#include "map/elevation_info.hpp"

#include "kml/serdes_common.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"
#include "geometry/simplification.hpp"

#include <optional>
#include <span>

ElevationInfo::ElevationInfo(std::vector<GeometryLine> const & lines)
{
  for (auto const & line : lines)
  {
    // Enclosing Track class should avoid empty geometries.
    // Put CHECK for the early crash report. Replace with ASSERT later.
    CHECK(!line.empty(), ());

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

void ElevationInfo::SmoothSlopeOutliers(double maxSlopePercent)
{
  double const maxSlope = maxSlopePercent / 100.0;
  double constexpr kZeroLength = 1.0E-9;

  for (auto & line : m_lines)
  {
    if (line.size() < 3)
      continue;

    // Mark outlier points if it is a peak/pit for both neighbors and a slope exceeds the limit.
    std::vector<bool> outlier(line.size(), false);
    for (size_t i = 1; i + 1 < line.size(); ++i)
    {
      double const distPrev = line[i].m_distance - line[i - 1].m_distance;
      double const distNext = line[i + 1].m_distance - line[i].m_distance;
      if (distPrev < kZeroLength || distNext < kZeroLength)
        continue;

      auto const deltaPrev = line[i].m_altitude - line[i - 1].m_altitude;
      auto const deltaNext = line[i + 1].m_altitude - line[i].m_altitude;
      double const slopePrev = fabs(double(deltaPrev) / distPrev);
      double const slopeNext = fabs(double(deltaNext) / distNext);

      // Point is an outlier if slope to at least one neighbor exceeds the limit
      // AND the slopes have opposite signs (spike pattern: up then down or vice versa).
      if (slopePrev > maxSlope || slopeNext > maxSlope)
      {
        // Opposite sign check (peak/pit): a real climb goes the same direction.
        if ((deltaPrev > 0) != (deltaNext > 0))
          outlier[i] = true;
      }
    }

    // Interpolate outlier altitudes from nearest non-outlier neighbors.
    for (size_t i = 1; i + 1 < line.size(); ++i)
    {
      if (!outlier[i])
        continue;

      // Find left non-outlier.
      size_t left = i - 1;
      while (left > 0 && outlier[left])
        --left;
      // Find right non-outlier.
      size_t right = i + 1;
      while (right + 1 < line.size() && outlier[right])
        ++right;

      double const totalDist = line[right].m_distance - line[left].m_distance;
      if (totalDist < kZeroLength)
        continue;

      double const f = (line[i].m_distance - line[left].m_distance) / totalDist;
      line[i].m_altitude =
          geometry::Altitude(double(line[left].m_altitude) * (1.0 - f) + double(line[right].m_altitude) * f);
    }
  }
}

double ElevationInfo::GetLength() const
{
  double length = 0;
  for (auto const & line : m_lines)
  {
    ASSERT(!line.empty(), ());
    length += line.back().m_distance;
  }
  return length;
}

size_t ElevationInfo::GetSize() const
{
  size_t size = 0;
  for (auto const & line : m_lines)
    size += line.size();
  return size;
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

namespace
{
struct EntryPosition
{
  size_t m_upper;
  double m_fraction;  // From the preceding sample to m_upper; 1 at an exact sample.
};

EntryPosition LocateEntry(std::span<ElevationInfo::Point const> points, double distance)
{
  distance = std::clamp(distance, 0.0, points.back().m_distance);
  auto const next = std::ranges::lower_bound(points, distance, {}, &ElevationInfo::Point::m_distance);
  size_t const index = next - points.begin();
  if (next->m_distance == distance)
    return {index, 1.0};

  auto const start = points[index - 1].m_distance;
  return {index, (distance - start) / (next->m_distance - start)};
}
}  // namespace

ElevationInfo::ChartSection ElevationInfo::GetChartSection(Section const & section) const
{
  auto const offset = [this](size_t line)
  {
    size_t count = 0;
    for (size_t i = 0; i < line; ++i)
      count += m_lines[i].size();
    return count;
  };
  auto const [upper, fraction] = LocateEntry(m_lines[section.m_entryLine], section.m_distanceOnLine);
  return {static_cast<double>(offset(section.m_entryLine) + upper) - (1.0 - fraction),
          offset(section.m_endLine) + (section.m_reverse ? 0 : m_lines[section.m_endLine].size() - 1)};
}

ElevationInfo::AltitudesInfo ElevationInfo::CalculateAltitudesInfo(Altitude threshold, Section const * section) const
{
  ASSERT(threshold >= 0, ());
  size_t const firstLine = section ? std::min(section->m_entryLine, section->m_endLine) : 0;
  size_t const endLine = section ? std::max(section->m_entryLine, section->m_endLine) + 1 : m_lines.size();
  ASSERT_LESS_OR_EQUAL(endLine, m_lines.size(), ());
  AltitudesInfo info;
  for (size_t i = firstLine; i < endLine; ++i)
  {
    auto points = std::span(m_lines[i]);
    ASSERT(!points.empty(), ());
    auto start = points.front().m_altitude;
    std::optional<Altitude> reverseEntry;
    if (section && i == section->m_entryLine)
    {
      // Chart smoothing and simplification can put the entry between two displayed heights.
      auto const [index, fraction] = LocateEntry(points, section->m_distanceOnLine);
      auto entry = points[index].m_altitude;
      if (fraction < 1.0)
        entry = static_cast<Altitude>(std::lround(std::lerp(points[index - 1].m_altitude, entry, fraction)));
      if (section->m_reverse)
      {
        reverseEntry = entry;
        points = points.first(index);
      }
      else
      {
        start = entry;
        points = points.subspan(index);
      }
    }

    Altitude previous = start;
    Altitude refAlt = start;
    info.m_minAltitude = std::min(info.m_minAltitude, start);
    info.m_maxAltitude = std::max(info.m_maxAltitude, start);

    auto const add = [&](Altitude alt)
    {
      info.m_minAltitude = std::min(info.m_minAltitude, alt);
      info.m_maxAltitude = std::max(info.m_maxAltitude, alt);
      auto const rawDelta = alt - previous;
      previous = alt;
      if (rawDelta > 0)
        info.m_totalAscentRaw += rawDelta;
      else
        info.m_totalDescentRaw += -rawDelta;

      // Filtered: threshold accumulation from reference point.
      auto const filteredDelta = alt - refAlt;
      if (filteredDelta > threshold)
      {
        info.m_totalAscentFiltered += filteredDelta;
        refAlt = alt;
      }
      else if (filteredDelta < -threshold)
      {
        info.m_totalDescentFiltered += -filteredDelta;
        refAlt = alt;
      }
    };
    for (auto const & point : points)
      add(point.m_altitude);
    if (reverseEntry)
      add(*reverseEntry);
  }
  // Apply the threshold in source order so reversing exchanges ascent and descent exactly.
  if (section && section->m_reverse)
  {
    std::swap(info.m_totalAscentRaw, info.m_totalDescentRaw);
    std::swap(info.m_totalAscentFiltered, info.m_totalDescentFiltered);
  }
  return info;
}

void GpsTrackElevation::AddGpsPoint(location::GpsInfo const & gps)
{
  // GPS track recording always produces a single line.
  auto const latLon = gps.GetLatLon();
  double distance = 0;
  if (m_lines.empty())
    m_lines.emplace_back();
  else
    distance = m_lines.back().back().m_distance + ms::DistanceOnEarth(m_lastLatLon, latLon);

  m_lines.back().emplace_back(distance, kml::ToAltitude(gps.m_altitude));
  m_lastLatLon = latLon;
}
