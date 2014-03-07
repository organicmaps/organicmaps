#include "alfa_animation_task.hpp"

#include "framework.hpp"

AlfaCompassAnim::AlfaCompassAnim(double start, double end, double timeInterval, double timeOffset, Framework * f)
  : m_start(start)
  , m_end(end)
  , m_current(start)
  , m_timeInterval(timeInterval)
  , m_timeOffset(timeOffset)
  , m_f(f)
{
}

bool AlfaCompassAnim::IsHiding() const
{
  return m_start > m_end;
}

float AlfaCompassAnim::GetCurrentAlfa() const
{
  return m_current;
}

void AlfaCompassAnim::OnStart(double ts)
{
  m_timeStart = ts;
  base_t::OnStart(ts);
  m_f->Invalidate();
}

void AlfaCompassAnim::OnStep(double ts)
{
  base_t::OnStep(ts);
  double elapsed = ts - (m_timeStart + m_timeOffset);
  if (elapsed >= 0.0)
  {
    double t = elapsed / m_timeInterval;
    if (t > 1.0)
    {
      m_current = m_end;
      End();
    }
    else
      m_current = m_start + t * (m_end - m_start);
  }

  m_f->Invalidate();
}
