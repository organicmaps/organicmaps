#pragma once

#include "drape_frontend/animation/base_interpolator.hpp"

namespace df
{
class OpacityAnimation : public BaseInterpolator
{
  using TBase = BaseInterpolator;

public:
  OpacityAnimation(double duration, double startOpacity, double endOpacity);
  OpacityAnimation(double duration, double delay, double startOpacity, double endOpacity);

  void Advance(double elapsedSeconds) override;
  double GetOpacity() const { return m_opacity; }

private:
  double m_startOpacity;
  double m_endOpacity;
  double m_opacity;
};
}  // namespace df
