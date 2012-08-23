#include "rotate_screen_task.hpp"
#include "framework.hpp"

RotateScreenTask::RotateScreenTask(Framework * framework,
                                   double startAngle,
                                   double endAngle,
                                   double speed)
  : m_framework(framework),
    m_startAngle(startAngle),
    m_endAngle(endAngle)
{
  m_startTime = 0;
  m_dist = ang::GetShortestDistance(m_startAngle, m_endAngle);
  m_interval = fabs(m_dist) / (2 * math::pi) * speed;
  m_endAngle = m_startAngle + m_dist;
}

void RotateScreenTask::OnStart(double ts)
{
  m_startTime = ts;
  anim::Task::OnStart(ts);
}

void RotateScreenTask::OnStep(double ts)
{
  if (ts - m_startTime > m_interval)
  {
    End();
    return;
  }

  if (!IsRunning())
    return;

  double elapsedSec = ts - m_startTime;
  double angle = m_startAngle + m_dist * elapsedSec / m_interval;

  m_framework->GetNavigator().SetAngle(angle);
}

void RotateScreenTask::OnEnd(double ts)
{
  /// ensuring that the final angle is reached
  m_framework->GetNavigator().SetAngle(m_endAngle);
}

double RotateScreenTask::EndAngle() const
{
  return m_endAngle;
}
