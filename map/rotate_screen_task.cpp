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
  m_isFinished = false;
}

void RotateScreenTask::OnStart(double ts)
{
  m_startTime = ts;
  m_curAngle = m_startAngle;
  m_framework->GetRenderPolicy()->SetIsAnimating(true);
}

void RotateScreenTask::OnStep(double ts)
{
  if (ts - m_startTime > m_interval)
  {
    m_isFinished = true;
    return;
  }

  if (IsFinished())
    return;

  double elapsedSec = ts - m_startTime;
  double angle = m_startAngle + (m_endAngle - m_startAngle) * elapsedSec / m_interval;

  m_framework->GetNavigator().Rotate(angle - m_curAngle);
  m_curAngle = angle;

  m_framework->GetRenderPolicy()->GetWindowHandle()->invalidate();
}

void RotateScreenTask::OnEnd(double ts)
{
  m_framework->GetRenderPolicy()->SetIsAnimating(false);
}

bool RotateScreenTask::IsFinished()
{
  return m_isFinished;
}

void RotateScreenTask::Finish()
{
  m_isFinished = true;
}
