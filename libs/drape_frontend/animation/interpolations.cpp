#include "drape_frontend/animation/interpolations.hpp"

#include "geometry/angles.hpp"

namespace df
{

double InterpolateDouble(double startV, double endV, double t)
{
  return startV + (endV - startV) * t;
}

m2::PointD InterpolatePoint(m2::PointD const & startPt, m2::PointD const & endPt, double t)
{
  m2::PointD diff = endPt - startPt;
  return startPt + diff * t;
}

double InterpolateAngle(double startAngle, double endAngle, double t)
{
  startAngle = ang::AngleIn2PI(startAngle);
  endAngle = ang::AngleIn2PI(endAngle);
  return startAngle + ang::GetShortestDistance(startAngle, endAngle) * t;
}

}  // namespace df
