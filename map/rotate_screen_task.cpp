#include "rotate_screen_task.hpp"
#include "framework.hpp"

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
  anim::AngleInterpolation::OnStep(ts);
  m_framework->GetNavigator().SetAngle(m_outAngle);
}

void RotateScreenTask::OnEnd(double ts)
{
  anim::AngleInterpolation::OnEnd(ts);
  m_framework->GetNavigator().SetAngle(m_outAngle);
}
