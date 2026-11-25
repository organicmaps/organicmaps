#pragma once

#include "drape_frontend/animation_constants.hpp"

#include "base/assert.hpp"

#include <algorithm>

namespace df
{
class Animation
{
public:
  bool IsFinished() const { return m_elapsed == m_duration; }
  double GetRemainingTime(AnimationClock::time_point now) const { return GetRemainingTime(now, m_duration); }
  void Advance(AnimationClock::time_point now)
  {
    m_elapsed = std::clamp(std::chrono::duration<double>(now - m_requestedAt).count(), 0.0, m_duration);
  }

protected:
  Animation(AnimationClock::time_point requestedAt, double duration) : m_requestedAt(requestedAt), m_duration(duration)
  {
    ASSERT_GREATER(duration, 0.0, ());
  }

  // Linear progress of a value that changes during the first duration seconds of the animation.
  static double GetTimeFraction(double elapsed, double duration)
  {
    return elapsed < duration ? elapsed / duration : 1.0;
  }
  double GetTimeFraction(double duration) const { return GetTimeFraction(m_elapsed, duration); }
  double GetTimeFraction() const { return GetTimeFraction(m_duration); }
  double GetElapsed() const { return m_elapsed; }
  // The time left for a value that changes during the first duration seconds of the animation.
  double GetRemainingTime(AnimationClock::time_point now, double duration) const
  {
    return std::max(0.0, duration - std::chrono::duration<double>(now - m_requestedAt).count());
  }

private:
  AnimationClock::time_point const m_requestedAt;
  double const m_duration;
  double m_elapsed = 0.0;
};
}  // namespace df
