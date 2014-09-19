#pragma once

#include "task.hpp"

namespace anim
{
  class AngleInterpolation : public Task
  {
  private:

    double m_startAngle;
    double m_curAngle;
    double & m_outAngle;
    double m_startTime;
    double m_endAngle;
    double m_interval;
    double m_dist;
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

    double EndAngle() const;
    void SetEndAngle(double val);

  private:
    void CalcParams(double start, double end, double speed);
  };
}
