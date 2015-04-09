#include "anim/angle_interpolation.hpp"
#include "anim/controller.hpp"

#include "geometry/angles.hpp"

#include "base/logging.hpp"


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

    double const elapsed = ts - m_startTime;
    if (elapsed >= m_interval)
    {
      End();
      return;
    }

    m_curAngle = m_outAngle = m_startAngle + elapsed * m_speed;

    Task::OnStep(ts);
  }

  void AngleInterpolation::OnEnd(double ts)
  {
    // ensuring that the final value was reached
    m_outAngle = m_endAngle;
    Task::OnEnd(ts);
  }

  void AngleInterpolation::SetEndAngle(double val)
  {
    CalcParams(m_curAngle, val, fabs(m_speed));
    m_startTime = GetController()->GetCurrentTime();
  }

  void AngleInterpolation::CalcParams(double start, double end, double speed)
  {
    ASSERT_GREATER(speed, 0.0, ());

    m_startAngle = start;
    m_speed = speed;
    m_startTime = 0.0;

    double const dist = ang::GetShortestDistance(start, end);
    if (dist < 0.0)
      m_speed = -m_speed;

    m_curAngle = m_startAngle;
    m_endAngle = m_startAngle + dist;
    m_interval = dist / m_speed;
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
