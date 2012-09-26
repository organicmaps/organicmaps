#include "angle_interpolation.hpp"

#include "../geometry/angles.hpp"

namespace anim
{
  AngleInterpolation::AngleInterpolation(double start,
                                         double end,
                                         double speed,
                                         double & out)
    : m_startAngle(start),
      m_outAngle(out)
  {
    m_speed = speed;
    m_startTime = 0;
    m_dist = ang::GetShortestDistance(start, end);
    m_curAngle = m_startAngle;
    m_endAngle = m_startAngle + m_dist;
    m_interval = fabs(m_dist) / (2 * math::pi) * m_speed;
  }

  void AngleInterpolation::OnStart(double ts)
  {
    m_startTime = ts;
    Task::OnStart(ts);
  }

  void AngleInterpolation::OnStep(double ts)
  {
    if (ts - m_startTime > m_interval)
    {
      End();
      return;
    }

    if (!IsRunning())
      return;

    double elapsedSec = ts - m_startTime;
    m_curAngle = m_outAngle = m_startAngle + m_dist * elapsedSec / m_interval;

    Task::OnStep(ts);
  }

  void AngleInterpolation::OnEnd(double ts)
  {
    // ensuring that the final value was reached
    m_outAngle = m_endAngle;
    Task::OnEnd(ts);
  }

  double AngleInterpolation::EndAngle() const
  {
    return m_endAngle;
  }

  void AngleInterpolation::SetEndAngle(double val)
  {
    m_startAngle = m_curAngle;
    m_dist = ang::GetShortestDistance(m_startAngle, val);
    m_endAngle = m_startAngle + m_dist;
    m_interval = fabs(m_dist) / (2 * math::pi) * m_speed;
  }
}
