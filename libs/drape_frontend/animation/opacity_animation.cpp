#include "drape_frontend/animation/opacity_animation.hpp"
#include "drape_frontend/animation/interpolations.hpp"

namespace df
{

OpacityAnimation::OpacityAnimation(double duration, double startOpacity, double endOpacity)
  : OpacityAnimation(duration, 0.0, startOpacity, endOpacity)
{}

OpacityAnimation::OpacityAnimation(double duration, double delay, double startOpacity, double endOpacity)
  : BaseInterpolator(duration, delay)
  , m_startOpacity(startOpacity)
  , m_endOpacity(endOpacity)
  , m_opacity(startOpacity)
{}

void OpacityAnimation::Advance(AnimationClock::time_point now)
{
  TBase::Advance(now);
  m_opacity = InterpolateDouble(m_startOpacity, m_endOpacity, GetT());
}

}  // namespace df
