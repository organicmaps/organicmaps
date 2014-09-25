#include "angle_interpolation.hpp"
#include "controller.hpp"

#include "../geometry/angles.hpp"
#include "../base/logging.hpp"

namespace anim
{
  AngleInterpolation::AngleInterpolation(double start,
                                         double end,
                                         double speed,
                                         double & out)
    : m_outAngle(out)
  {
    CalcParams(start, end, speed);
  }

  void AngleInterpolation::Reset(double start, double end, double speed)
  {
    CalcParams(start, end, speed);
    m_startTime = GetController()->GetCurrentTime();
    m_outAngle = m_startAngle;
    SetState(EReady);
  }

  void AngleInterpolation::OnStart(double ts)
  {
    m_startTime = ts;
    m_outAngle = m_startAngle;
    Task::OnStart(ts);
  }

  void AngleInterpolation::OnStep(double ts)
  {
    if (!IsRunning())
      return;

    if (ts - m_startTime >= m_interval)
    {
      End();
      return;
    }

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
    CalcParams(m_curAngle, val, m_speed);
    m_startTime = GetController()->GetCurrentTime();
  }

  void AngleInterpolation::CalcParams(double start, double end, double speed)
  {
    m_startAngle = start;
    m_speed = speed;
    m_startTime = 0;
    m_dist = ang::GetShortestDistance(start, end);
    m_curAngle = m_startAngle;
    m_endAngle = m_startAngle + m_dist;
    m_interval = fabs(m_dist) / (2 * math::pi) * m_speed;
  }

  SafeAngleInterpolation::SafeAngleInterpolation(double start, double end, double speed)
    : TBase(start, end, speed, m_angle)
  {
    m_angle = start;
  }

  void SafeAngleInterpolation::ResetDestParams(double dstAngle, double speed)
  {
    Reset(GetCurrentValue(), dstAngle, speed);
  }

  double SafeAngleInterpolation::GetCurrentValue() const
  {
    return m_angle;
  }

}
