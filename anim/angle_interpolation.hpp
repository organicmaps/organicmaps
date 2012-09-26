#pragma once

#include "task.hpp"
#include "../std/function.hpp"

namespace anim
{
  class AngleInterpolation : public Task
  {
  public:

    typedef function<void()> TCallback;

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

    void OnStart(double ts);
    void OnStep(double ts);
    void OnEnd(double ts);

    double EndAngle() const;
    void SetEndAngle(double val);
  };
}
