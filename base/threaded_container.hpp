#pragma once

#include "base/cancellable.hpp"
#include "base/condition.hpp"
#include "base/timer.hpp"

struct ThreadedContainer : public base::Cancellable
{
protected:

  base::Timer m_Timer;

  mutable threads::Condition m_Cond;

public:
  /// Cancellable overrides:
  void Cancel() override;
};
