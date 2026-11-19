#pragma once

#include "drape_frontend/animation/animation.hpp"

#include "base/macros.hpp"

namespace df
{
class BaseInterpolator : public Animation
{
public:
  explicit BaseInterpolator(double duration, double delay = 0.0);
  virtual ~BaseInterpolator();
  DISALLOW_COPY_AND_MOVE(BaseInterpolator);

  virtual void Advance(AnimationClock::time_point now) { Animation::Advance(now); }

protected:
  double GetT() const { return GetTimeFraction(); }
};
}  // namespace df
