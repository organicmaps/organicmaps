#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/any_rect2d.hpp"

namespace df
{

double InterpolateDouble(double startV, double endV, double t);
m2::PointD InterpolatePoint(m2::PointD const & startPt, m2::PointD const & endPt, double t);

class InerpolateAngle
{
public:
  InerpolateAngle(double startAngle, double endAngle);
  double Interpolate(double t) const;

private:
  double m_startAngle;
  double m_delta;
};

} // namespace df
