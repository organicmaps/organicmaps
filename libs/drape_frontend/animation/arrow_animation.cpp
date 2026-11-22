#include "drape_frontend/animation/arrow_animation.hpp"

#include "drape_frontend/animation/interpolations.hpp"

namespace df
{
ArrowAnimation::ArrowAnimation(ArrowPose const & start, ArrowPose const & end, AnimationClock::time_point requestedAt,
                               double duration)
  : Animation(requestedAt, duration)
  , m_start(start)
  , m_end(end)
{}

ArrowPose ArrowAnimation::GetPose() const
{
  double const progress = GetProgress();
  return {InterpolatePoint(m_start.m_position, m_end.m_position, progress),
          InterpolateAngle(m_start.m_azimuth, m_end.m_azimuth, progress)};
}
}  // namespace df
