#include "anim/value_interpolation.hpp"

namespace anim
{
  ValueInterpolation::ValueInterpolation(double start,
                                         double end,
                                         double interval,
                                         double & out)
    : m_startValue(start),
      m_outValue(out),
      m_endValue(end)
  {
    m_dist = end - start;
    m_interval = interval;
  }

  void ValueInterpolation::OnStart(double ts)
  {
    m_startTime = ts;
    m_outValue = m_startValue;
    Task::OnStart(ts);
  }

  void ValueInterpolation::OnStep(double ts)
  {
    if (ts - m_startTime >= m_interval)
    {
      End();
      return;
    }

    if (!IsRunning())
      return;

    double elapsedSec = ts - m_startTime;
    m_outValue = m_startValue + m_dist * elapsedSec / m_interval;

    Task::OnStep(ts);
  }

  void ValueInterpolation::OnEnd(double ts)
  {
    // ensuring that the final value was reached
    m_outValue = m_endValue;
    Task::OnEnd(ts);
  }
}

