#pragma once

#include "drape_frontend/animation/base_interpolator.hpp"
#include "drape_frontend/animation/interpolations.hpp"

#include "geometry/any_rect2d.hpp"
#include "geometry/screenbase.hpp"

namespace df
{

class BaseModelViewAnimation : public BaseInterpolator
{
public:
  BaseModelViewAnimation(double duration, double delay = 0) : BaseInterpolator(duration, delay) {}

  virtual m2::AnyRectD GetCurrentRect() const = 0;
  virtual m2::AnyRectD GetTargetRect() const = 0;
};

class ModelViewAnimation : public BaseModelViewAnimation
{
public:
  static double GetRotateDuration(double startAngle, double endAngle);
  static double GetMoveDuration(m2::PointD const & startPt, m2::PointD const & endPt, ScreenBase const & convertor);
  static double GetScaleDuration(double startSize, double endSize);

  /// aDuration - angleDuration
  /// mDuration - moveDuration
  /// sDuration - scaleDuration
  ModelViewAnimation(m2::AnyRectD const & startRect, m2::AnyRectD const & endRect,
                     double aDuration, double mDuration, double sDuration);
  m2::AnyRectD GetCurrentRect() const override;
  m2::AnyRectD GetTargetRect() const override;

private:
  m2::AnyRectD GetRect(double elapsedTime) const;

private:
  InerpolateAngle m_angleInterpolator;
  m2::PointD m_startZero, m_endZero;
  m2::RectD m_startRect, m_endRect;

  double m_angleDuration;
  double m_moveDuration;
  double m_scaleDuration;
};

} // namespace df
