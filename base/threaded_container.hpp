#pragma once

#include "base/cancellable.hpp"
#include "base/condition.hpp"
#include "base/timer.hpp"

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
