#include "drape_frontend/animation/opacity_animation.hpp"
#include "drape_frontend/animation/interpolations.hpp"

namespace df
{

OpacityAnimation::OpacityAnimation(double duration, double startOpacity, double endOpacity)
  : BaseInterpolator(duration)
  , m_opacity(1.0)
  , m_startOpacity(startOpacity)
  , m_endOpacity(endOpacity)
{
}

void OpacityAnimation::Advance(double elapsedSeconds)
{
  TBase::Advance(elapsedSeconds);
  m_opacity = InterpolateDouble(m_startOpacity, m_endOpacity, GetT());
}

}
