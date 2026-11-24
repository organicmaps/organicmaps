#pragma once

#include "drape_frontend/animation_constants.hpp"

#include "base/macros.hpp"

#include <set>

namespace df
{
class OpacityAnimation;

class InterpolationHolder
{
public:
  static InterpolationHolder & Instance();
  bool IsActive() const;
  void Advance(AnimationClock::time_point now);

private:
  InterpolationHolder() = default;
  ~InterpolationHolder();
  DISALLOW_COPY_AND_MOVE(InterpolationHolder);

private:
  friend class OpacityAnimation;
  void Register(OpacityAnimation * interpolator);
  void Deregister(OpacityAnimation * interpolator);

  using TInterpolatorSet = std::set<OpacityAnimation *>;
  TInterpolatorSet m_interpolations;
};
}  // namespace df
