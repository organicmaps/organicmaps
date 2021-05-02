#pragma once

#include "base/cancellable.hpp"
#include "base/timer.hpp"

#include <condition_variable>
#include <mutex>

struct ThreadedContainer : public base::Cancellable
{
protected:

  base::Timer m_Timer;

  mutable std::mutex m_condLock;
  mutable std::condition_variable m_Cond;

public:
  /// Cancellable overrides:
  void Cancel() override;
};
