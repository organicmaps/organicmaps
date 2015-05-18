#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/any_rect2d.hpp"

namespace df
{

double InterpolateDouble(double startV, double endV, double t);
m2::PointD InterpolatePoint(m2::PointD const & startPt, m2::PointD const & endPt, double t);
m2::RectD InterpolateRect(m2::RectD const & startRect, m2::RectD const & endRect, double t);

class InerpolateAngle
{
public:
  InerpolateAngle(double startAngle, double endAngle);
  double Interpolate(double t);

private:
  double m_startAngle;
  double m_delta;
};

class InterpolateAnyRect
{
public:
  InterpolateAnyRect(m2::AnyRectD const & startRect, m2::AnyRectD const & endRect);
  m2::AnyRectD Interpolate(double  t);

private:
  m2::PointD m_startZero, m_endZero;
  InerpolateAngle m_angleInterpolator;
  m2::RectD m_startRect, m_endRect;
};

} // namespace df
