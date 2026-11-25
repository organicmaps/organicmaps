#include "drape_frontend/animation/interpolations.hpp"

#include "geometry/angles.hpp"

namespace df
{

double InterpolateDouble(double startV, double endV, double t)
{
  if (t == 1.0)
    return endV;
  return startV + (endV - startV) * t;
}

m2::PointD InterpolatePoint(m2::PointD const & startPt, m2::PointD const & endPt, double t)
{
  if (t == 1.0)
    return endPt;
  return startPt + (endPt - startPt) * t;
}

double InterpolateAngle(double startAngle, double endAngle, double t)
{
  if (t == 1.0 || startAngle == endAngle)
    return endAngle;
  if (t == 0.0)
    return startAngle;
  startAngle = ang::AngleIn2PI(startAngle);
  endAngle = ang::AngleIn2PI(endAngle);
  return startAngle + ang::GetShortestDistance(startAngle, endAngle) * t;
}

}  // namespace df
