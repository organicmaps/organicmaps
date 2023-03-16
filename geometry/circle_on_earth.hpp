#pragma once

#include "geometry/latlon.hpp"
#include "geometry/point2d.hpp"

#include <vector>

namespace ms
{
std::vector<m2::PointD> CreateCircleGeometryOnEarth(ms::LatLon const & center, double radiusMeters,
                                                    double angleStepDegree);
}  // namespace ms
