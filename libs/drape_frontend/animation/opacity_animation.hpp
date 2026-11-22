#pragma once

#include "drape_frontend/animation/animation.hpp"

#include "base/macros.hpp"

namespace df
{
class OpacityAnimation : public Animation
{
public:
  OpacityAnimation(double duration, double startOpacity, double endOpacity);
  ~OpacityAnimation();
  DISALLOW_COPY_AND_MOVE(OpacityAnimation);

  double GetOpacity() const;

private:
  double const m_startOpacity;
  double const m_endOpacity;
};
}  // namespace df
