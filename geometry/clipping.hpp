#pragma once

#include "rect2d.hpp"
#include "spline.hpp"
#include "triangle2d.hpp"

#include "std/function.hpp"

namespace m2
{

using ClipTriangleByRectResultIt = function<void(m2::PointD const &, m2::PointD const &, m2::PointD const &)>;

void ClipTriangleByRect(m2::RectD const & rect, m2::PointD const & p1,
                        m2::PointD const & p2, m2::PointD const & p3,
                        ClipTriangleByRectResultIt const & resultIterator);

vector<m2::SharedSpline> ClipSplineByRect(m2::RectD const & rect, m2::SharedSpline const & spline);

} // namespace m2
