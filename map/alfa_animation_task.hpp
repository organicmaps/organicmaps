#pragma once

#include "anim/task.hpp"


class Framework;

class AlfaAnimationTask : public anim::Task
{
  typedef anim::Task BaseT;

public:
  AlfaAnimationTask(double start, double end,
                    double timeInterval, double timeOffset,
                    Framework * f);

  bool IsHiding() const;
  float GetCurrentAlfa() const;

  virtual void OnStart(double ts);
  virtual void OnStep(double ts);

private:
  double m_start;
  double m_end;
  double m_current;
  double m_timeInterval;
  double m_timeOffset;
  double m_timeStart;

  Framework * m_f;
};
