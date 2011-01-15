#pragma once

#include "../../geometry/rect2d.hpp"

#include "../../std/string.hpp"
#include "../../std/vector.hpp"

#include <boost/polygon/polygon.hpp>

namespace boost
{
  namespace polygon
  {
    template <>
    struct coordinate_traits<uint32_t> {
         typedef uint32_t coordinate_type;
         typedef long double area_type;
         typedef int64_t manhattan_area_type;
         typedef uint64_t unsigned_area_type;
         typedef int64_t coordinate_difference;
         typedef long double coordinate_distance;
    };
  }
}

namespace kml
{

  typedef uint32_t TCoordType;
  typedef boost::polygon::polygon_data<TCoordType> Polygon;
  typedef std::vector<Polygon> PolygonSet;

  struct CountryPolygons
  {
    CountryPolygons(string const & name = "") : m_name(name) {}
    PolygonSet m_polygons;
    string m_name;
    m2::RectD m_rect;
  };

  void LoadPolygonsFromKml(string const & kmlFile, CountryPolygons & country);

}
