#include "generator/routing_city_boundaries_processor.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"

#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include "std/boost_geometry.hpp"

namespace generator
{

double AreaOnEarth(std::vector<m2::PointD> const & points)
{
  namespace bg = boost::geometry;
  using LonLatCoords = bg::cs::spherical_equatorial<bg::degree>;
  bg::model::ring<bg::model::point<double, 2, LonLatCoords>> sphericalPolygon;

  auto const addPoint = [&sphericalPolygon](auto const & point)
  {
    auto const latlon = mercator::ToLatLon(point);
    sphericalPolygon.emplace_back(latlon.m_lon, latlon.m_lat);
  };

  sphericalPolygon.reserve(points.size());
  for (auto point : points)
    addPoint(point);

  addPoint(points.front());

  bg::strategy::area::spherical<> areaCalculationStrategy(ms::kEarthRadiusMeters);

  double const area = bg::area(sphericalPolygon, areaCalculationStrategy);
  return fabs(area);
}

}  // namespace generator
