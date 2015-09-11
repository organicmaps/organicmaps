#include "model_view_animation.hpp"

namespace df
{

ModelViewAnimation::ModelViewAnimation(m2::AnyRectD const & startRect, m2::AnyRectD const & endRect,
                                       double aDuration, double mDuration, double sDuration)
  : BaseModelViewAnimation(max(max(aDuration, mDuration), sDuration))
  , m_angleInterpolator(startRect.Angle().val(), endRect.Angle().val())
  , m_startZero(startRect.GlobalZero())
  , m_endZero(endRect.GlobalZero())
  , m_startRect(startRect.GetLocalRect())
  , m_endRect(endRect.GetLocalRect())
  , m_angleDuration(aDuration)
  , m_moveDuration(mDuration)
  , m_scaleDuration(sDuration)
{
}

m2::AnyRectD ModelViewAnimation::GetCurrentRect(ScreenBase const & screen) const
{
  return GetRect(GetElapsedTime());
}

m2::AnyRectD ModelViewAnimation::GetTargetRect(ScreenBase const & screen) const
{
  return GetRect(GetDuration());
}

namespace
{

double GetSafeT(double elapsed, double duration)
{
  if (duration <= 0.0 || elapsed > duration)
    return 1.0;

  return elapsed / duration;
}

} // namespace

m2::AnyRectD ModelViewAnimation::GetRect(double elapsedTime) const
{
  double dstAngle = m_angleInterpolator.Interpolate(GetSafeT(elapsedTime, m_angleDuration));
  m2::PointD dstZero = InterpolatePoint(m_startZero, m_endZero, GetSafeT(elapsedTime, m_moveDuration));
  m2::RectD dstRect = InterpolateRect(m_startRect, m_endRect, GetSafeT(elapsedTime, m_scaleDuration));

  return m2::AnyRectD(dstZero, dstAngle, dstRect);
}

double ModelViewAnimation::GetRotateDuration(double startAngle, double endAngle)
{
  return 0.5 * fabs(ang::GetShortestDistance(startAngle, endAngle)) / math::pi;
}

namespace
{

double CalcAnimSpeedDuration(double pxDiff, double pxSpeed)
{
  if (my::AlmostEqualAbs(pxDiff, 0.0, 1e-5))
    return 0.0;

  return fabs(pxDiff) / pxSpeed;
}

}

double ModelViewAnimation::GetMoveDuration(m2::PointD const & startPt, m2::PointD const & endPt, ScreenBase const & convertor)
{
  m2::RectD const & dispPxRect = convertor.PixelRect();
  double pixelLength = convertor.GtoP(endPt).Length(convertor.GtoP(startPt));
  if (pixelLength < 1e-5)
    return 0.0;

  if (pixelLength < 0.2 * min(dispPxRect.SizeX(), dispPxRect.SizeY()))
    return 0.2;

  double const pixelSpeed = 1.5 * min(dispPxRect.SizeX(), dispPxRect.SizeY());
  return CalcAnimSpeedDuration(pixelLength, pixelSpeed);
}

double ModelViewAnimation::GetScaleDuration(double startSize, double endSize)
{
  if (startSize > endSize)
    swap(startSize, endSize);

  // Resize 2.0 times should be done for 0.5 seconds.
  static double const pixelSpeed = 2.0 / 0.5;
  return CalcAnimSpeedDuration(endSize / startSize, pixelSpeed);
}

FixedPointAnimation::FixedPointAnimation(m2::AnyRectD const & startRect, m2::AnyRectD const & endRect,
                                         double aDuration, double mDuration, double sDuration,
                                         m2::PointD const & pixelPoint, m2::PointD const & globalPoint)
  : ModelViewAnimation(startRect, endRect, aDuration, mDuration, sDuration)
  , m_pixelPoint(pixelPoint)
  , m_globalPoint(globalPoint)
{
}

void FixedPointAnimation::ApplyFixedPoint(ScreenBase const & screen, m2::AnyRectD & rect) const
{
  ScreenBase s = screen;
  s.SetFromRect(rect);
  m2::PointD const p = s.PtoG(m_pixelPoint);
  rect.Offset(m_globalPoint - p);
}

m2::AnyRectD FixedPointAnimation::GetCurrentRect(ScreenBase const & screen) const
{
  m2::AnyRectD r = GetRect(GetElapsedTime());
  ApplyFixedPoint(screen, r);
  return r;
}

m2::AnyRectD FixedPointAnimation::GetTargetRect(ScreenBase const & screen) const
{
  m2::AnyRectD r = GetRect(GetDuration());
  ApplyFixedPoint(screen, r);
  return r;
}

FollowAndRotateAnimation::FollowAndRotateAnimation(m2::AnyRectD const & startRect, m2::PointD const & userPos,
                                                   double newCenterOffset, double oldCenterOffset,
                                                   double azimuth, double duration)
  : BaseModelViewAnimation(duration)
  , m_angleInterpolator(startRect.Angle().val(), azimuth)
  , m_rect(startRect.GetLocalRect())
  , m_userPos(userPos)
  , m_newCenterOffset(newCenterOffset)
  , m_oldCenterOffset(oldCenterOffset)
{}

m2::AnyRectD FollowAndRotateAnimation::GetCurrentRect(ScreenBase const & screen) const
{
  return GetRect(GetElapsedTime());
}

m2::AnyRectD FollowAndRotateAnimation::GetTargetRect(ScreenBase const & screen) const
{
  return GetRect(GetDuration());
}

m2::AnyRectD FollowAndRotateAnimation::GetRect(double elapsedTime) const
{
  double const t = GetSafeT(elapsedTime, GetDuration());
  double const azimuth = m_angleInterpolator.Interpolate(t);
  double const centerOffset = InterpolateDouble(m_oldCenterOffset, m_newCenterOffset, t);

  m2::PointD viewVector = m_userPos.Move(1.0, azimuth + math::pi2) - m_userPos;
  viewVector.Normalize();
  m2::PointD centerPos = m_userPos + (viewVector * centerOffset);
  return m2::AnyRectD(centerPos, azimuth, m_rect);
}

} // namespace df
