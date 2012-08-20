#include "rotate_screen_task.hpp"
#include "framework.hpp"

RotateScreenTask::RotateScreenTask(Framework * framework,
                                   double startAngle,
                                   double endAngle,
                                   double interval)
  : m_framework(framework),
    m_startAngle(startAngle),
    m_endAngle(endAngle),
    m_interval(interval)
{
  m_startTime = 0;
}

void RotateScreenTask::OnStart(double ts)
{
  m_startTime = ts;
  m_curAngle = m_startAngle;
  anim::Task::OnStart(ts);
}

void RotateScreenTask::OnStep(double ts)
{
  if (ts - m_startTime > m_interval)
  {
    End();
    return;
  }

  if (IsEnded())
    return;

  double elapsedSec = ts - m_startTime;
  double angle = m_startAngle + (m_endAngle - m_startAngle) * elapsedSec / m_interval;

  m_framework->GetNavigator().Rotate(angle - m_curAngle);
  m_curAngle = angle;
}

void RotateScreenTask::OnEnd(double ts)
{
  /// ensuring that the final angle is reached
  m_framework->GetNavigator().SetAngle(m_endAngle);
}
