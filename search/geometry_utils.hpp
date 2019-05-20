#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

namespace search
{
// Distance between 2 mercator points in meters.
double PointDistance(m2::PointD const & a, m2::PointD const & b);

// Tests whether two rects given in the mercator projection are
// equal with the absolute precision |eps|.
bool IsEqualMercator(m2::RectD const & r1, m2::RectD const & r2, double eps);
}  // namespace search
