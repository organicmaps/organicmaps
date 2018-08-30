#pragma once

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "base/visitor.hpp"

#include <string>
#include <vector>

namespace routing_quality
{
namespace mapbox
{
using LonLat = std::vector<double>;
using Coordinates = std::vector<LonLat>;

struct Geometry
{
  DECLARE_VISITOR(visitor(m_coordinates, "coordinates"),
                  visitor(m_type, "type"))

  void Build(std::vector<m2::PointD> const & from)
  {
    m_coordinates.reserve(from.size());
    for (auto const & p : from)
    {
      auto const ll = MercatorBounds::ToLatLon(p);
      m_coordinates.push_back({ll.lon, ll.lat});
    }

    m_type = "LineString";
  }

  Coordinates m_coordinates;
  std::string m_type;
};

struct Route
{
  DECLARE_VISITOR(visitor(m_geometry, "geometry"),
                  visitor(m_duration, "duration"))

  Geometry m_geometry;
  double m_duration = 0.0;
};

struct DirectionsResponse
{
  DECLARE_VISITOR(visitor(m_routes, "routes"))

  std::vector<Route> m_routes;
};
}  // namespace mapbox
}  // namespace routing_quality
