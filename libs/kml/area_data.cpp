#include "kml/area_data.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"

#include "base/math.hpp"

#include <algorithm>

#include <boost/geometry/algorithms/correct.hpp>
#include <boost/geometry/algorithms/is_valid.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>

namespace kml
{
namespace
{
bool NormalizeRing(AreaRing & ring)
{
  for (auto const & point : ring)
  {
    auto const & p = point.m_point;
    if (!math::is_finite(p.x) || !math::is_finite(p.y) || p.x < ms::LatLon::kMinLon || p.x > ms::LatLon::kMaxLon ||
        p.y < ms::LatLon::kMinLat || p.y > ms::LatLon::kMaxLat ||
        (point.m_altitude && !math::is_finite(*point.m_altitude)))
      return false;
  }
  ring.erase(std::unique(ring.begin(), ring.end()), ring.end());
  if (ring.size() > 1 && ring.front().m_point == ring.back().m_point)
  {
    if (ring.front() != ring.back())
      return false;
    ring.pop_back();
  }
  return ring.size() >= 3;
}

}  // namespace

bool NormalizeAreaGeometry(std::vector<AreaPolygon> & polygons)
{
  namespace bg = boost::geometry;
  using Point = bg::model::d2::point_xy<double>;
  using Polygon = bg::model::polygon<Point, false, false>;
  if (polygons.empty())
    return false;

  for (auto & polygon : polygons)
  {
    if (!NormalizeRing(polygon.m_outer))
      return false;
    Polygon validation;
    auto const copyRing = [](AreaRing const & from, auto & to, double reference)
    {
      to.reserve(from.size());
      double minX = mercator::NearestWrapX(from.front().m_point.x, reference);
      double maxX = minX;
      for (auto const & point : from)
      {
        auto const x = mercator::NearestWrapX(point.m_point.x, reference);
        to.emplace_back(x, point.m_point.y);
        minX = std::min(minX, x);
        maxX = std::max(maxX, x);
        reference = x;
      }
      return (minX + maxX) / 2;
    };
    auto const outerReference = polygon.m_outer.front().m_point.x;
    // Holes share the shell's longitude frame, including shells wider than half the world.
    auto const holeReference = copyRing(polygon.m_outer, validation.outer(), outerReference);
    validation.inners().reserve(polygon.m_holes.size());
    for (auto & hole : polygon.m_holes)
    {
      if (!NormalizeRing(hole))
        return false;
      copyRing(hole, validation.inners().emplace_back(), holeReference);
    }
    bg::correct(validation);
    if (!bg::is_valid(validation))
      return false;

    // Mirror Boost's reversals on full vertices to retain exact source coordinates and altitude.
    auto const orientRing = [](AreaRing & ring, auto const & corrected, double reference)
    {
      auto const & first = ring.front().m_point;
      if (mercator::NearestWrapX(first.x, reference) != corrected.front().x() || first.y != corrected.front().y())
        std::reverse(ring.begin(), ring.end());
    };
    orientRing(polygon.m_outer, validation.outer(), outerReference);
    for (size_t i = 0; i < polygon.m_holes.size(); ++i)
      orientRing(polygon.m_holes[i], validation.inners()[i], holeReference);
  }
  return true;
}

bool AreaData::operator==(AreaData const & other) const
{
  return m_id == other.m_id && m_name == other.m_name && m_description == other.m_description &&
         m_style == other.m_style && m_visible == other.m_visible && IsEqual(m_timestamp, other.m_timestamp) &&
         m_properties == other.m_properties && m_polygons == other.m_polygons;
}

std::string DebugPrint(AreaData const & area)
{
  return "AreaData [id=" + std::to_string(area.m_id) + ", polygons=" + std::to_string(area.m_polygons.size()) + "]";
}
}  // namespace kml
