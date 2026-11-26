#pragma once

#include "drape_frontend/animation_constants.hpp"

#include "base/assert.hpp"

#include <algorithm>

namespace df
{
class Animation
{
public:
  double GetRemainingTime(AnimationClock::time_point now) const { return std::max(0.0, m_duration - Elapsed(now)); }
  bool IsFinished() const { return m_timeFraction == 1.0; }
  void Advance(AnimationClock::time_point now) { m_timeFraction = std::clamp(Elapsed(now) / m_duration, 0.0, 1.0); }

protected:
  Animation(AnimationClock::time_point requestedAt, double duration, double maxDuration = kMaxAnimationTimeSec)
    : m_requestedAt(requestedAt)
    , m_duration(duration)
  {
    ASSERT(duration > 0.0 && duration <= maxDuration, (duration, maxDuration));
  }

  double GetTimeFraction() const { return m_timeFraction; }
  double GetProgress() const
  {
    double const remaining = 1.0 - m_timeFraction;
    return 1.0 - remaining * remaining * remaining;
  }

private:
  double Elapsed(AnimationClock::time_point now) const
  {
    return std::chrono::duration<double>(now - m_requestedAt).count();
  }

  AnimationClock::time_point const m_requestedAt;
  double const m_duration;
  double m_timeFraction = 0.0;
};
}  // namespace df
