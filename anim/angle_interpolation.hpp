#pragma once

#include "anim/task.hpp"

namespace anim
{
  class AngleInterpolation : public Task
  {
    double m_startAngle;
    double m_curAngle;
    double & m_outAngle;
    double m_startTime;
    double m_endAngle;
    double m_interval;
    double m_speed;

  public:
    AngleInterpolation(double start,
                       double end,
                       double speed,
                       double & out);

    void Reset(double start, double end, double speed);

    void OnStart(double ts);
    void OnStep(double ts);
    void OnEnd(double ts);

    void SetEndAngle(double val);

  private:
    void CalcParams(double start, double end, double speed);
  };

  class SafeAngleInterpolation : public AngleInterpolation
  {
    typedef AngleInterpolation TBase;

  public:
    SafeAngleInterpolation(double start, double end, double speed);

    void ResetDestParams(double dstAngle, double speed);
    double GetCurrentValue() const;

  private:
    double m_angle;
  };
}
