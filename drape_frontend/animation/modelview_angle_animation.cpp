#include "modelview_angle_animation.hpp"

namespace df
{

ModelViewAngleAnimation::ModelViewAngleAnimation(double startAngle, double endAngle, double duration)
  : BaseModeViewAnimation(duration)
  , m_angle(startAngle, endAngle)
{
}

void ModelViewAngleAnimation::Apply(Navigator & navigator)
{
  double resultAngle = m_angle.Interpolate(GetT());
  m2::AnyRectD r = navigator.Screen().GlobalRect();
  r.SetAngle(resultAngle);
  navigator.SetFromRect(r);
}

}
