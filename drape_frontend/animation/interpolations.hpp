#pragma once

#include "geometry/point2d.hpp"

namespace df
{
  m2::PointD InterpolatePoint(m2::PointD const & startPt, m2::PointD const & endPt, double t);

  class InerpolateAngle
  {
  public:
    InerpolateAngle(double startAngle, double endAngle);
    double Interpolate(double t);

  private:
    double m_startAngle;
    double m_delta;
  };
} // namespace df
