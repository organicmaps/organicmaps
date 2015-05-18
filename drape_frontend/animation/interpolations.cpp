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

m2::RectD InterpolateRect(m2::RectD const & startRect, m2::RectD const & endRect, double t)
{
  m2::PointD center = InterpolatePoint(startRect.Center(), endRect.Center(), t);
  double halfSizeX = 0.5 * InterpolateDouble(startRect.SizeX(), endRect.SizeX(), t);
  double halfSizeY = 0.5 * InterpolateDouble(startRect.SizeY(), endRect.SizeY(), t);

  return m2::RectD(center.x - halfSizeX, center.y - halfSizeY,
                   center.x + halfSizeX, center.y + halfSizeY);
}

InerpolateAngle::InerpolateAngle(double startAngle, double endAngle)
{
  m_startAngle = ang::AngleIn2PI(startAngle);
  m_delta = ang::GetShortestDistance(m_startAngle, ang::AngleIn2PI(endAngle));
}

double InerpolateAngle::Interpolate(double t) const
{
  return m_startAngle + m_delta * t;
}

InterpolateAnyRect::InterpolateAnyRect(m2::AnyRectD const & startRect, m2::AnyRectD const & endRect)
  : m_startZero(startRect.GlobalZero())
  , m_endZero(endRect.GlobalZero())
  , m_angleInterpolator(startRect.Angle().val(), endRect.Angle().val())
  , m_startRect(startRect.GetLocalRect())
  , m_endRect(endRect.GetLocalRect())
{
}

m2::AnyRectD InterpolateAnyRect::Interpolate(double t) const
{
  double angle = m_angleInterpolator.Interpolate(t);
  m2::PointD zero = InterpolatePoint(m_startZero, m_endZero, t);
  m2::RectD rect = InterpolateRect(m_startRect, m_endRect, t);

  return m2::AnyRectD(zero, angle, rect);
}

} // namespace df
