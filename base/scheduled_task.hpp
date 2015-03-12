#pragma once

#include "thread.hpp"
#include "condition.hpp"

#include "../std/function.hpp"
#include "../std/unique_ptr.hpp"


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
    virtual void Cancel();
  };

  /// The construction and destruction order is strict here: m_cond is
  /// used by m_routine and m_routine is used by m_thread.
  threads::Condition m_cond;
  unique_ptr<Routine> const m_routine;
  threads::Thread m_thread;

public:
  /// Constructor by function and time in miliseconds.
  ScheduledTask(fn_t const & fn, unsigned ms);

  ~ScheduledTask();

  /// @name Task could be cancelled before time elapses.
  //@{
  /// @return false If the task is already running or in some intermediate state.
  bool CancelNoBlocking();
  void CancelBlocking();
  //@}
};
