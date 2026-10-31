#include "kml/area_data.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace kml
{
namespace
{
// NaN and infinities are filtered out by the parsers (strings::to_double, JSON syntax), so only reasonable
// ranges are checked here: with -ffast-math, the range checks can't reject NaN.
bool IsValidPoint(AreaPoint const & point)
{
  auto const & p = point.m_latLon;
  return mercator::ValidLon(p.m_lon) && mercator::ValidLat(p.m_lat) &&
         (!point.m_altitude || math::is_finite(*point.m_altitude));
}

// Returns 1 for a counterclockwise ring and -1 for a clockwise one, or 0 if rounding errors may change
// the sign of its area, like for a bow-tie with cancelling lobes.
int AreaSign(AreaRing const & ring)
{
  auto const & origin = ring.front().m_latLon;
  double area = 0;
  double magnitude = 0;
  for (size_t i = 2; i < ring.size(); ++i)
  {
    auto const & a = ring[i - 1].m_latLon;
    auto const & b = ring[i].m_latLon;
    double const p = (a.m_lon - origin.m_lon) * (b.m_lat - origin.m_lat);
    double const q = (a.m_lat - origin.m_lat) * (b.m_lon - origin.m_lon);
    area += p - q;
    magnitude += std::abs(p) + std::abs(q);
  }
  // Twice the bound of rounding errors for any order of operations, which -ffast-math may change.
  double const error = 4 * ring.size() * std::numeric_limits<double>::epsilon() * magnitude;
  return area > error ? 1 : (area < -error ? -1 : 0);
}

// Unlike a zero area, it doesn't drop self-crossing rings with lobes of opposite windings.
bool IsCollinear(AreaRing const & ring)
{
  auto const & a = ring.front().m_latLon;
  auto const it = std::ranges::find_if(ring, [&a](AreaPoint const & point) { return point.m_latLon != a; });
  if (it == ring.end())
    return true;
  auto const & b = it->m_latLon;
  return std::ranges::all_of(ring, [&](AreaPoint const & point)
  {
    auto const & c = point.m_latLon;
    return (b.m_lon - a.m_lon) * (c.m_lat - a.m_lat) == (b.m_lat - a.m_lat) * (c.m_lon - a.m_lon);
  });
}

// A-B-A backtracking, unlike vertices that differ only in height.
bool IsSpike(AreaPoint const & a, AreaPoint const & b, AreaPoint const & c)
{
  return a.m_latLon == c.m_latLon && b.m_latLon != c.m_latLon;
}

// Removes consecutive identical vertices, exact A-B-A backtracking and the closing vertex.
// Vertices that differ only in height and nonconsecutive repeated vertices are retained.
// Clears a degenerate ring.
void NormalizeRing(AreaRing & ring, bool counterclockwise)
{
  if (ring.size() > 1 && ring.front().m_latLon == ring.back().m_latLon &&
      ring.front().m_altitude != ring.back().m_altitude)
  {
    LOG(LWARNING, ("Polygon closing height differs; using the first vertex's height"));
    ring.back().m_altitude = ring.front().m_altitude;
  }

  size_t size = 0;
  for (auto const & point : ring)
  {
    if (size > 1 && IsSpike(ring[size - 2], ring[size - 1], point))
      --size;
    if (size == 0 || !(ring[size - 1] == point))
      ring[size++] = point;
  }

  // The same across the start of the ring, which may repeat at the end.
  size_t first = 0;
  while (size - first > 2)
    if (ring[first] == ring[size - 1])
      --size;
    else if (IsSpike(ring[size - 1], ring[first], ring[first + 1]))
      ++first;
    else if (IsSpike(ring[size - 2], ring[size - 1], ring[first]))
      --size;
    else
      break;
  ring.resize(size);
  ring.erase(ring.begin(), ring.begin() + first);

  if (ring.size() < 3 || IsCollinear(ring))
  {
    ring.clear();
    return;
  }
  // A reversed ring can't get the opposite sign, so the winding is stable after save and load,
  // and a ring with an uncertain sign keeps its order.
  int const sign = AreaSign(ring);
  if (sign != 0 && (sign > 0) != counterclockwise)
    std::reverse(ring.begin(), ring.end());
}
}  // namespace

bool NormalizeAreaGeometry(std::vector<AreaPolygon> & polygons)
{
  for (auto const & polygon : polygons)
  {
    if (!std::ranges::all_of(polygon.m_outer, IsValidPoint))
      return false;
    for (auto const & hole : polygon.m_holes)
      if (!std::ranges::all_of(hole, IsValidPoint))
        return false;
  }

  for (auto & polygon : polygons)
  {
    NormalizeRing(polygon.m_outer, true /* counterclockwise */);
    if (polygon.m_outer.empty())
      continue;
    for (auto & hole : polygon.m_holes)
      NormalizeRing(hole, false /* counterclockwise */);
    std::erase_if(polygon.m_holes, [](AreaRing const & hole) { return hole.empty(); });
  }
  std::erase_if(polygons, [](AreaPolygon const & polygon) { return polygon.m_outer.empty(); });
  return !polygons.empty();
}

bool AreaData::operator==(AreaData const & other) const
{
  return m_id == other.m_id && m_name == other.m_name && m_description == other.m_description &&
         m_style == other.m_style && m_visible == other.m_visible && IsEqual(m_timestamp, other.m_timestamp) &&
         m_properties == other.m_properties && m_polygons == other.m_polygons &&
         m_isMultiPolygon == other.m_isMultiPolygon && m_altitudeIsWgs84 == other.m_altitudeIsWgs84;
}

std::string DebugPrint(AreaData const & area)
{
  return "AreaData [id=" + std::to_string(area.m_id) + ", polygons=" + std::to_string(area.m_polygons.size()) + "]";
}
}  // namespace kml
