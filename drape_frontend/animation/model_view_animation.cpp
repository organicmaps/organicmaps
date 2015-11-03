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
  double const kRotateDurationScalar = 0.75;

  return kRotateDurationScalar * fabs(ang::GetShortestDistance(startAngle, endAngle)) / math::pi;
}

namespace
{

double CalcAnimSpeedDuration(double pxDiff, double pxSpeed)
{
  double const kEps = 1e-5;

  if (my::AlmostEqualAbs(pxDiff, 0.0, kEps))
    return 0.0;

  return fabs(pxDiff) / pxSpeed;
}

}

double ModelViewAnimation::GetMoveDuration(m2::PointD const & startPt, m2::PointD const & endPt, ScreenBase const & convertor)
{
  double const kMinMoveDuration = 0.2;
  double const kMinSpeedScalar = 0.2;
  double const kMaxSpeedScalar = 7.0;
  double const kEps = 1e-5;

  m2::RectD const & dispPxRect = convertor.PixelRect();
  double const pixelLength = convertor.GtoP(endPt).Length(convertor.GtoP(startPt));
  if (pixelLength < kEps)
    return 0.0;

  double const minSize = min(dispPxRect.SizeX(), dispPxRect.SizeY());
  if (pixelLength < kMinSpeedScalar * minSize)
    return kMinMoveDuration;

  double const pixelSpeed = kMaxSpeedScalar * minSize;
  return CalcAnimSpeedDuration(pixelLength, pixelSpeed);
}

double ModelViewAnimation::GetScaleDuration(double startSize, double endSize)
{
  // Resize 2.0 times should be done for 0.3 seconds.
  double constexpr kPixelSpeed = 2.0 / 0.3;

  if (startSize > endSize)
    swap(startSize, endSize);

  return CalcAnimSpeedDuration(endSize / startSize, kPixelSpeed);
}

ScaleAnimation::ScaleAnimation(m2::AnyRectD const & startRect, m2::AnyRectD const & endRect,
                               double aDuration, double mDuration, double sDuration,
                               m2::PointD const & globalPoint, m2::PointD const & pixelOffset)
  : ModelViewAnimation(startRect, endRect, aDuration, mDuration, sDuration)
  , m_globalPoint(globalPoint)
  , m_pixelOffset(pixelOffset)
{
}

void ScaleAnimation::ApplyPixelOffset(ScreenBase const & screen, m2::AnyRectD & rect) const
{
  ScreenBase s = screen;
  s.SetFromRect(rect);

  m2::PointD const pixelPoint = s.GtoP(m_globalPoint);
  m2::PointD const newCenter = s.PtoG(pixelPoint + m_pixelOffset);

  rect = m2::AnyRectD(newCenter, rect.Angle(), rect.GetLocalRect());
}

m2::AnyRectD ScaleAnimation::GetCurrentRect(ScreenBase const & screen) const
{
  m2::AnyRectD r = GetRect(GetElapsedTime());
  ApplyPixelOffset(screen, r);
  return r;
}

m2::AnyRectD ScaleAnimation::GetTargetRect(ScreenBase const & screen) const
{
  m2::AnyRectD r = GetRect(GetDuration());
  ApplyPixelOffset(screen, r);
  return r;
}

FollowAndRotateAnimation::FollowAndRotateAnimation(m2::AnyRectD const & startRect,
                                                   m2::RectD const & targetLocalRect,
                                                   m2::PointD const & userPos,
                                                   m2::PointD const & startPixelPos,
                                                   m2::PointD const & endPixelPos,
                                                   double azimuth, double duration)
  : BaseModelViewAnimation(duration)
  , m_angleInterpolator(startRect.Angle().val(), -azimuth)
  , m_rect(startRect.GetLocalRect())
  , m_target(targetLocalRect)
  , m_userPos(userPos)
  , m_startPixelPos(startPixelPos)
  , m_endPixelPos(endPixelPos)
{}

m2::AnyRectD FollowAndRotateAnimation::GetCurrentRect(ScreenBase const & screen) const
{
  return GetRect(screen, GetElapsedTime());
}

m2::AnyRectD FollowAndRotateAnimation::GetTargetRect(ScreenBase const & screen) const
{
  return GetRect(screen, GetDuration());
}

m2::PointD FollowAndRotateAnimation::CalculateCenter(ScreenBase const & screen, m2::PointD const & userPos,
                                                     m2::PointD const & pixelPos, double azimuth)
{
  return CalculateCenter(screen.GlobalRect().GetLocalRect(), screen.PixelRect(), userPos, pixelPos, azimuth);
}

m2::PointD FollowAndRotateAnimation::CalculateCenter(m2::RectD const & localRect, m2::RectD const & pixelRect,
                                                     m2::PointD const & userPos, m2::PointD const & pixelPos,
                                                     double azimuth)
{
  m2::PointD formingVector = pixelRect.Center() - pixelPos;
  formingVector.x *= (localRect.SizeX() / pixelRect.SizeX());
  formingVector.y *= (localRect.SizeY() / pixelRect.SizeY());
  double const centerOffset = formingVector.Length();

  m2::PointD viewVector = userPos.Move(1.0, azimuth + math::pi2) - userPos;
  viewVector.Normalize();
  return userPos + (viewVector * centerOffset);
}

m2::AnyRectD FollowAndRotateAnimation::GetRect(ScreenBase const & screen, double elapsedTime) const
{
  double const t = GetSafeT(elapsedTime, GetDuration());
  double const azimuth = m_angleInterpolator.Interpolate(t);
  m2::RectD const currentRect = InterpolateRect(m_rect, m_target, t);
  m2::PointD const pixelPos = InterpolatePoint(m_startPixelPos, m_endPixelPos, t);
  m2::PointD const centerPos = CalculateCenter(currentRect, screen.PixelRect(), m_userPos, pixelPos, azimuth);

  return m2::AnyRectD(centerPos, azimuth, currentRect);
}

} // namespace df
