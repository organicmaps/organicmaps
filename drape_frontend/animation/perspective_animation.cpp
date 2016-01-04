#include "perspective_animation.hpp"
#include "drape_frontend/animation/interpolations.hpp"

namespace df
{

// static
double PerspectiveAnimation::GetRotateDuration(double startAngle, double endAngle)
{
  return 0.5 * fabs(endAngle - startAngle) / math::pi4;
}

PerspectiveAnimation::PerspectiveAnimation(double duration, double startRotationAngle, double endRotationAngle)
  : PerspectiveAnimation(duration, 0.0 /* delay */, startRotationAngle, endRotationAngle)
{
}

PerspectiveAnimation::PerspectiveAnimation(double duration, double delay, double startRotationAngle, double endRotationAngle)
  : BaseInterpolator(duration, delay)
  , m_startRotationAngle(startRotationAngle)
  , m_endRotationAngle(endRotationAngle)
  , m_rotationAngle(startRotationAngle)
{
}

void PerspectiveAnimation::Advance(double elapsedSeconds)
{
  TBase::Advance(elapsedSeconds);
  m_rotationAngle = InterpolateDouble(m_startRotationAngle, m_endRotationAngle, GetT());
}

} // namespace df
