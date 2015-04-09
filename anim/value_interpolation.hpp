#pragma once

#include "anim/task.hpp"

namespace anim
{
  class ValueInterpolation : public Task
  {
  private:

    double m_startValue;
    double & m_outValue;
    double m_endValue;

    double m_startTime;
    double m_interval;
    double m_dist;

  public:

    ValueInterpolation(double start,
                       double end,
                       double interval,
                       double & out);

    void OnStart(double ts);
    void OnStep(double ts);
    void OnEnd(double ts);
  };
}
