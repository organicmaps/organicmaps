#pragma once

#include "drape_frontend/screen_animations.hpp"

#include <deque>

namespace df
{
class KineticScroller
{
public:
  void Init(ScreenBase const & modelView, AnimationClock::time_point now);
  void Update(ScreenBase const & modelView, AnimationClock::time_point now);
  bool IsActive() const;
  void Cancel();
  std::optional<MapAnimation> CreateKineticAnimation(ScreenBase const & modelView, AnimationClock::time_point now);

private:
  std::pair<m2::PointD, double> GetDirectionAndVelocity(ScreenBase const & modelView,
                                                        AnimationClock::time_point now) const;

  static double GetDurationSeconds(AnimationClock::time_point const & t2, AnimationClock::time_point const & t1)
  {
    return std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
  }

  std::deque<std::pair<m2::PointD, AnimationClock::time_point>> m_points;
  bool m_isActive = false;
};
}  // namespace df
