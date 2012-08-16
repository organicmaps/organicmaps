#pragma once

#include "../anim/task.hpp"

class Framework;

class RotateScreenTask : public anim::Task
{
private:

  Framework * m_framework;

  double m_startTime;
  double m_startAngle;
  double m_endAngle;
  double m_interval;
  double m_curAngle;

  bool m_isFinished;

public:

  RotateScreenTask(Framework * framework,
                   double startAngle,
                   double endAngle,
                   double interval);

  void OnStart(double ts);
  void OnStep(double ts);
  void OnEnd(double ts);

  bool IsFinished();
  void Finish();
};
