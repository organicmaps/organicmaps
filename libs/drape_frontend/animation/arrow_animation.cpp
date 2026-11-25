#include "drape_frontend/animation/arrow_animation.hpp"

#include "drape_frontend/animation/interpolations.hpp"

namespace df
{
ArrowAnimation::ArrowAnimation(ArrowPose const & start, ArrowPose const & end, AnimationClock::time_point requestedAt,
                               double moveDuration, double rotateDuration)
  : Animation(requestedAt, std::max(moveDuration, rotateDuration))
  , m_start(start)
  , m_end(end)
  , m_moveDuration(moveDuration)
  , m_rotateDuration(rotateDuration)
{}

ArrowPose ArrowAnimation::GetPose() const
{
  return {InterpolatePoint(m_start.m_position, m_end.m_position, GetTimeFraction(m_moveDuration)),
          InterpolateAngle(m_start.m_azimuth, m_end.m_azimuth, GetTimeFraction(m_rotateDuration))};
}
}  // namespace df
