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
    fn_t m_Fn;
    size_t m_Interval;
    threads::Condition m_Cond;
  public:

    Routine(fn_t const & fn, size_t ms);

    void Do();
  };

private:

  threads::Thread m_Thread;

public:

  /// Constructor by function and time in miliseconds.
  ScheduledTask(fn_t const& fn, size_t ms);
  /// Task could be cancelled before time elapses.
  void Cancel();
};
