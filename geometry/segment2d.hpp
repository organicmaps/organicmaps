#pragma once

#include "geometry/point2d.hpp"

namespace m2
{
bool IsPointOnSegmentEps(m2::PointD const & pt, m2::PointD const & p1, m2::PointD const & p2,
                         double eps);
bool IsPointOnSegment(m2::PointD const & pt, m2::PointD const & p1, m2::PointD const & p2);
}  // namespace m2
