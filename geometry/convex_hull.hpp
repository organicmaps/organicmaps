#pragma once

#include "geometry/point2d.hpp"

#include <vector>

namespace m2
{
// Builds a convex hull around |points|.
//
// Complexity: O(n * log(n)), where n is the number of points.
std::vector<PointD> BuildConvexHull(std::vector<PointD> points);
}  // namespace m2
