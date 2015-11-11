#pragma once

#include "drape_frontend/animation/base_interpolator.hpp"

namespace df
{

class PerspectiveAnimation : public BaseInterpolator
{
  using TBase = BaseInterpolator;

public:
  static double GetRotateDuration(double startAngle, double endAngle);

  PerspectiveAnimation(double duration, double startRotationAngle, double endRotationAngle);
  PerspectiveAnimation(double duration, double delay, double startRotationAngle, double endRotationAngle);

  void Advance(double elapsedSeconds) override;
  double GetRotationAngle() const { return m_rotationAngle; }

private:
  double m_startRotationAngle;
  double m_endRotationAngle;
  double m_rotationAngle;
};

} // namespace df

