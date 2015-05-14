#include "drape_frontend/animation/interpolations.hpp"

#include "geometry/angles.hpp"

namespace df
{

m2::PointD InterpolatePoint(m2::PointD const & startPt, m2::PointD const & endPt, double t)
{
  m2::PointD diff = endPt - startPt;
  return startPt + diff * t;
}

InerpolateAngle::InerpolateAngle(double startAngle, double endAngle)
{
  m_startAngle = ang::AngleIn2PI(startAngle);
  m_delta = ang::GetShortestDistance(m_startAngle, ang::AngleIn2PI(endAngle));
}

double InerpolateAngle::Interpolate(double t)
{
  return m_startAngle + m_delta * t;
}


} // namespace df
