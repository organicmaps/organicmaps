#pragma once

#include "geometry/point2d.hpp"

namespace df
{

double InterpolateDouble(double startV, double endV, double t);
m2::PointD InterpolatePoint(m2::PointD const & startPt, m2::PointD const & endPt, double t);
double InterpolateAngle(double startAngle, double endAngle, double t);
// The size of a marker that pops up, see kPopUpTimeSec.
double GetPopUpScale(double t);

}  // namespace df
