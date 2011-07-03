#pragma once

#include "condition.hpp"
#include "timer.hpp"

struct ThreadedContainer
{
protected:

  my::Timer m_Timer;
  double m_WaitTime;

  threads::Condition m_Cond;
  bool m_IsCancelled;

public:

  ThreadedContainer();

  void Cancel();
  bool IsCancelled() const;

  double WaitTime() const;
};
