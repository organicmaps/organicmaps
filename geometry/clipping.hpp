#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/spline.hpp"

#include <functional>
#include <vector>

namespace m2
{
using ClipTriangleByRectResultIt = std::function<void(m2::PointD const &, m2::PointD const &, m2::PointD const &)>;

void ClipTriangleByRect(m2::RectD const & rect, m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3,
                        ClipTriangleByRectResultIt const & resultIterator);

void ClipPathByRect(m2::RectD const & rect, std::vector<m2::PointD> && path,
                    std::function<void(m2::SharedSpline &&)> const & fn);
std::vector<m2::SharedSpline> ClipSplineByRect(m2::RectD const & rect, m2::SharedSpline const & spline);

using GuidePointsForSmooth = std::vector<std::pair<m2::PointD, m2::PointD>>;
void ClipPathByRectBeforeSmooth(m2::RectD const & rect, std::vector<m2::PointD> const & path,
                                GuidePointsForSmooth & guidePoints,
                                std::vector<std::vector<m2::PointD>> & clippedPaths);
}  // namespace m2
