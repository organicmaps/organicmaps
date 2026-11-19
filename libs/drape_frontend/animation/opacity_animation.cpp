#include "drape_frontend/animation/opacity_animation.hpp"

#include "drape_frontend/animation/interpolation_holder.hpp"
#include "drape_frontend/animation/interpolations.hpp"

namespace df
{
OpacityAnimation::OpacityAnimation(double duration, double startOpacity, double endOpacity)
  : Animation(AnimationClock::now(), duration)
  , m_startOpacity(startOpacity)
  , m_endOpacity(endOpacity)
{
  InterpolationHolder::Instance().Register(this);
}

OpacityAnimation::~OpacityAnimation()
{
  InterpolationHolder::Instance().Deregister(this);
}

double OpacityAnimation::GetOpacity() const
{
  return InterpolateDouble(m_startOpacity, m_endOpacity, GetTimeFraction());
}
}  // namespace df
