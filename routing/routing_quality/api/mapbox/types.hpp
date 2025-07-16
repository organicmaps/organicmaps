#pragma once

#include "routing/routing_quality/api/api.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "base/visitor.hpp"

#include <string>
#include <vector>

namespace routing_quality
{
namespace api
{
namespace mapbox
{
using LonLat = std::vector<double>;

struct Geometry
{
  DECLARE_VISITOR(visitor(m_coordinates, "coordinates"))
  std::vector<LonLat> m_coordinates;
};

struct Route
{
  DECLARE_VISITOR(visitor(m_geometry, "geometry"), visitor(m_duration, "duration"), visitor(m_distance, "distance"))

  Geometry m_geometry;
  double m_duration = 0.0;
  double m_distance = 0.0;
};

struct MapboxResponse
{
  DECLARE_VISITOR(visitor(m_routes, "routes"))

  ResultCode m_code = ResultCode::Error;
  std::vector<Route> m_routes;
};
}  // namespace mapbox
}  // namespace api
}  // namespace routing_quality
