#pragma once

#include "geometry/point2d.hpp"

#include <vector>

namespace generator
{
double AreaOnEarth(std::vector<m2::PointD> const & points);
}  // namespace generator
