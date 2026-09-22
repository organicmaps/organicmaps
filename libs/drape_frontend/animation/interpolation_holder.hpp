#pragma once

#include "drape/render_context.hpp"

#include "base/macros.hpp"

#include <set>

namespace df
{
class BaseInterpolator;

class InterpolationHolder
{
  friend class dp::RenderContext;

public:
  static InterpolationHolder & Instance();
  bool IsActive() const;
  void Advance(double elapsedSeconds);

private:
  InterpolationHolder() = default;
  ~InterpolationHolder();
  DISALLOW_COPY_AND_MOVE(InterpolationHolder);

private:
  friend class BaseInterpolator;
  void RegisterInterpolator(BaseInterpolator * interpolator);
  void DeregisterInterpolator(BaseInterpolator * interpolator);

  using TInterpolatorSet = std::set<BaseInterpolator *>;
  TInterpolatorSet m_interpolations;
};
}  // namespace df
