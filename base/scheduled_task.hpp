#pragma once

#include "thread.hpp"
#include "condition.hpp"

#include "../std/function.hpp"
#include "../std/scoped_ptr.hpp"


/// Class, which performs any function when the specified
/// amount of time is elapsed.
class ScheduledTask
{
  typedef function<void()> fn_t;

  class Routine : public threads::IRoutine
  {
    fn_t m_fn;
    unsigned m_ms;
    threads::Condition * m_pCond;

  public:
    Routine(fn_t const & fn, unsigned ms, threads::Condition * cond);
    virtual void Do();
  };

  scoped_ptr<Routine> m_routine;
  threads::Thread m_thread;
  threads::Condition m_cond;

public:
  /// Constructor by function and time in miliseconds.
  ScheduledTask(fn_t const & fn, unsigned ms);

  /// Task could be cancelled before time elapses. This function is NON-blocking.
  /// @return false If the task is already running.
  bool Cancel();
};
