#pragma once

#include "cancellable.hpp"
#include "condition.hpp"
#include "timer.hpp"

struct ThreadedContainer : public my::Cancellable
{
protected:

  my::Timer m_Timer;
  double m_WaitTime;

  mutable threads::Condition m_Cond;

public:
  ThreadedContainer();

  /// Cancellable overrides:
  void Cancel() override;

  double WaitTime() const;
};
