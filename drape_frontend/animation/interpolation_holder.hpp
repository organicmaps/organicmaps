#pragma once

#include "base/macros.hpp"

#include "std/set.hpp"

namespace df
{

class BaseInterpolator;
class InterpolationHolder
{
public:
  static InterpolationHolder & Instance();
  bool Advance(double elapsedSeconds);

private:
  InterpolationHolder() = default;
  ~InterpolationHolder();
  DISALLOW_COPY_AND_MOVE(InterpolationHolder);

private:
  friend class BaseInterpolator;
  void RegisterInterpolator(BaseInterpolator * interpolator);
  void DeregisterInterpolator(BaseInterpolator * interpolator);

  using TInterpolatorSet = set<BaseInterpolator *>;
  TInterpolatorSet m_interpolations;
};

}
