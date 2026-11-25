#pragma once

#include "geometry/point2d.hpp"

namespace df
{

double InterpolateDouble(double startV, double endV, double t);
m2::PointD InterpolatePoint(m2::PointD const & startPt, m2::PointD const & endPt, double t);
double InterpolateAngle(double startAngle, double endAngle, double t);
// A marker that pops up grows to 130%, shrinks to 80% and settles at 100% of its size.
double GetPopUpScale(double t);

}  // namespace df
