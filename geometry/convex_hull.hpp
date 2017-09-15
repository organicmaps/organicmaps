#pragma once

#include "geometry/point2d.hpp"

#include <vector>

namespace m2
{
// Builds a convex hull around |points|.  The hull polygon points are
// listed in the order of a counterclockwise traversal with no three
// points lying on the same straight line.
//
// Complexity: O(n * log(n)), where n is the number of points.
std::vector<PointD> BuildConvexHull(std::vector<PointD> points);
}  // namespace m2
