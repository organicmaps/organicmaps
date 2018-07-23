#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/spline.hpp"
#include "geometry/triangle2d.hpp"

#include <functional>
#include <vector>

namespace m2
{
using ClipTriangleByRectResultIt =
    std::function<void(m2::PointD const &, m2::PointD const &, m2::PointD const &)>;

void ClipTriangleByRect(m2::RectD const & rect, m2::PointD const & p1, m2::PointD const & p2,
                        m2::PointD const & p3, ClipTriangleByRectResultIt const & resultIterator);

std::vector<m2::SharedSpline> ClipSplineByRect(m2::RectD const & rect,
                                               m2::SharedSpline const & spline);
}  // namespace m2
