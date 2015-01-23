#include "map/alfa_animation_task.hpp"
#include "map/framework.hpp"


AlfaAnimationTask::AlfaAnimationTask(double start, double end,
                                     double timeInterval, double timeOffset,
                                     Framework * f)
  : m_start(start)
  , m_end(end)
  , m_current(start)
  , m_timeInterval(timeInterval)
  , m_timeOffset(timeOffset)
  , m_f(f)
{
}

bool AlfaAnimationTask::IsHiding() const
{
  return m_start > m_end;
}

float AlfaAnimationTask::GetCurrentAlfa() const
{
  return m_current;
}

void AlfaAnimationTask::OnStart(double ts)
{
  m_timeStart = ts;
  BaseT::OnStart(ts);
  ///@TODO UVR
  //m_f->Invalidate();
}

void AlfaAnimationTask::OnStep(double ts)
{
  BaseT::OnStep(ts);

  double const elapsed = ts - (m_timeStart + m_timeOffset);
  if (elapsed >= 0.0)
  {
    double const t = elapsed / m_timeInterval;
    if (t > 1.0)
    {
      m_current = m_end;
      End();
    }
    else
      m_current = m_start + t * (m_end - m_start);
  }

  ///@TODO UVR
  //m_f->Invalidate();
}
