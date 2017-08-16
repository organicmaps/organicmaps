#pragma once

#include "animation_system.hpp"

#include "drape/pointers.hpp"

#include "geometry/any_rect2d.hpp"

#include <chrono>

namespace df
{
class KineticScroller
{
public:
  void Init(ScreenBase const &modelView);
  bool IsActive() const;
  void Cancel();
  drape_ptr<Animation> CreateKineticAnimation(ScreenBase const & modelView);

private:
  m2::PointD GetDirection(ScreenBase const & modelView) const;

  std::chrono::steady_clock::time_point m_lastTimestamp;
  bool m_isActive = false;
  m2::AnyRectD m_lastRect;
};
}  // namespace df
