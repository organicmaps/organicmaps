#include "map/rotate_screen_task.hpp"
#include "map/framework.hpp"

RotateScreenTask::RotateScreenTask(Framework * framework,
                                   double startAngle,
                                   double endAngle,
                                   double speed)
  : anim::AngleInterpolation(startAngle,
                             endAngle,
                             speed,
                             m_outAngle),
    m_framework(framework)
{
}

void RotateScreenTask::OnStep(double ts)
{
  double prevAngle = m_outAngle;
  anim::AngleInterpolation::OnStep(ts);
  Navigator & nav = m_framework->GetNavigator();
  nav.SetAngle(nav.Screen().GetAngle() + m_outAngle - prevAngle);
  ///@TODO UVR
  //m_framework->Invalidate();
}

void RotateScreenTask::OnEnd(double ts)
{
  anim::AngleInterpolation::OnEnd(ts);
  Navigator & nav = m_framework->GetNavigator();
  nav.SetAngle(m_outAngle);
  ///@TODO UVR
  //m_framework->Invalidate();
}

bool RotateScreenTask::IsVisual() const
{
  return true;
}
