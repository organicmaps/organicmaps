#pragma once

#include "../anim/task.hpp"

class Framework;

class RotateScreenTask : public anim::Task
{
private:

  Framework * m_framework;

  double m_startTime;
  double m_startAngle;
  double m_dist;
  double m_endAngle;
  double m_interval;

public:

  RotateScreenTask(Framework * framework,
                   double startAngle,
                   double endAngle,
                   double interval);

  void OnStart(double ts);
  void OnStep(double ts);
  void OnEnd(double ts);

  double EndAngle() const;
};
