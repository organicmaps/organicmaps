#pragma once

#include "geometry/any_rect2d.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

namespace df
{

double InterpolateDouble(double startV, double endV, double t);
m2::PointD InterpolatePoint(m2::PointD const & startPt, m2::PointD const & endPt, double t);
double InterpolateAngle(double startAngle, double endAngle, double t);

}  // namespace df
