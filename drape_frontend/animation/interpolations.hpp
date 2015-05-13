#pragma once

#include "geometry/point2d.hpp"

namespace df
{
  m2::PointD Interpolate(m2::PointD const & startPt, m2::PointD const & endPt, double t);
} // namespace df
