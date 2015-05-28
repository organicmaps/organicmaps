#include "model_view_animation.hpp"

namespace df
{

namespace
{

double GetMoveDuration(m2::PointD const & startPx, m2::PointD const & endPt, int minDisplaySize)
{
  return min(0.5, 0.5 * startPx.Length(endPt) / 50.0);
}

double GetRotateDuration(double const & startAngle, double const & endAngle)
{
  return 0.5 * fabs(ang::GetShortestDistance(startAngle, endAngle) / math::twicePi);
}

double GetScaleDuration(m2::RectD const & startRect, m2::RectD const & endRect)
{
  double const startSizeX = startRect.SizeX();
  double const startSizeY = startRect.SizeY();
  double const endSizeX = endRect.SizeX();
  double const endSizeY = endRect.SizeY();

  double const maxSizeX = max(startSizeX, endSizeX);
  double const minSizeX = min(startSizeX, endSizeX);
  double const maxSizeY = max(startSizeY, endSizeY);
  double const minSizeY = min(startSizeY, endSizeY);

  return min(minSizeX / maxSizeX, minSizeY / maxSizeY);
}

} // namespace

ModelViewAnimation::ModelViewAnimation(m2::AnyRectD const & startRect, m2::AnyRectD const & endRect, double duration)
  : BaseInterpolator(duration)
  , m_interpolator(startRect, endRect)
{
}

m2::AnyRectD ModelViewAnimation::GetCurrentRect() const
{
  return m_interpolator.Interpolate(GetT());
}

m2::AnyRectD ModelViewAnimation::GetTargetRect() const
{
  return m_interpolator.Interpolate(1.0);
}

double ModelViewAnimation::GetDuration(m2::AnyRectD const & startRect, m2::AnyRectD const & endRect,
                                       ScreenBase const & convertor)
{
  m2::PointD const pxStartZero(convertor.GtoP(startRect.GlobalZero()));
  m2::PointD const pxEndZero(convertor.GtoP(endRect.GlobalZero()));

  m2::RectD const & pixelRect = convertor.PixelRect();
  double moveDuration = GetMoveDuration(pxStartZero, pxEndZero, min(pixelRect.SizeX(), pixelRect.SizeY()));
  double rotateDuration = GetRotateDuration(startRect.Angle().val(), endRect.Angle().val());
  double scaleDuration = GetScaleDuration(startRect.GetLocalRect(), endRect.GetLocalRect());

  return max(scaleDuration, max(moveDuration, rotateDuration));
}

} // namespace df
