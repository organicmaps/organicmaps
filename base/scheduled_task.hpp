#pragma once

#include "../std/function.hpp"

#include "thread.hpp"
#include "condition.hpp"

/// Class, which performs any function when the specified
/// amount of time is elapsed.
class ScheduledTask
{
public:

  typedef function<void()> fn_t;

  class Routine : public threads::IRoutine
  {
  private:
    fn_t m_fn;
    unsigned m_ms;
    threads::Condition * m_pCond;
  public:

    Routine(fn_t const & fn, unsigned ms, threads::Condition * cond);

    void Do();
    void Cancel();
  };

private:

  threads::Thread m_thread;
  threads::Condition m_cond;

public:

  /// Constructor by function and time in miliseconds.
  ScheduledTask(fn_t const & fn, unsigned ms);
  /// Task could be cancelled before time elapses.
  void Cancel();
};
