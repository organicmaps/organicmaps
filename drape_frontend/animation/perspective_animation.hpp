#pragma once

#include "drape_frontend/animation/base_interpolator.hpp"

namespace df
{

class PerspectiveAnimation : public BaseInterpolator
{
  using TBase = BaseInterpolator;

public:
  PerspectiveAnimation(double duration, double startRotationAngle, double endRotationAngle);
  PerspectiveAnimation(double duration, double delay, double startRotationAngle, double endRotationAngle);

  static double GetRotateDuration(double startAngle, double endAngle);

  void Advance(double elapsedSeconds) override;
  double GetRotationAngle() const { return m_rotationAngle; }

private:
  double const m_startRotationAngle;
  double const m_endRotationAngle;
  double m_rotationAngle;
};

} // namespace df
