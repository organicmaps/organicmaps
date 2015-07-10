#include "model_view_animation.hpp"

namespace df
{

ModelViewAnimation::ModelViewAnimation(m2::AnyRectD const & startRect, m2::AnyRectD const & endRect,
                                       double aDuration, double mDuration, double sDuration)
  : BaseInterpolator(max(max(aDuration, mDuration), sDuration))
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

m2::AnyRectD ModelViewAnimation::GetCurrentRect() const
{
  return GetRect(GetElapsedTime());
}

m2::AnyRectD ModelViewAnimation::GetTargetRect() const
{
  return GetRect(GetDuration());
}

namespace
{

double GetSafeT(double elapsed, double duration)
{
  if (my::AlmostEqualAbs(duration, 0.0, 1e-5))
    return 0.0;
  return elapsed / duration;
}

} // namespace

m2::AnyRectD ModelViewAnimation::GetRect(double elapsedTime) const
{
  double const angleElapsed = min(elapsedTime, m_angleDuration);
  double const moveElapsed = min(elapsedTime, m_moveDuration);
  double const scaleElapsed = min(elapsedTime, m_scaleDuration);

  double dstAngle = m_angleInterpolator.Interpolate(GetSafeT(angleElapsed, m_angleDuration));
  m2::PointD dstZero = InterpolatePoint(m_startZero, m_endZero, GetSafeT(moveElapsed, m_moveDuration));
  m2::RectD dstRect = InterpolateRect(m_startRect, m_endRect, GetSafeT(scaleElapsed, m_scaleDuration));

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
  if (pixelLength < 0.2 * min(dispPxRect.SizeX(), dispPxRect.SizeY()))
    return 0.2;

  double const pixelSpeed = 1.5 * min(dispPxRect.SizeX(), dispPxRect.SizeY());
  return CalcAnimSpeedDuration(pixelLength, pixelSpeed);
}

double ModelViewAnimation::GetScaleDuration(double startSize, double endSize)
{
  if (startSize > endSize)
    swap(startSize, endSize);

  double const SCALE_FACTOR = 2.0;
  double const ONE_DIV_SCALE_TIME = 2.0;

  static double const pixelSpeed = ONE_DIV_SCALE_TIME * SCALE_FACTOR;
  return CalcAnimSpeedDuration(endSize / startSize, pixelSpeed);
}

} // namespace df
