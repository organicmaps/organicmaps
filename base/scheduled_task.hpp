#pragma once

#include "base/condition.hpp"
#include "base/thread.hpp"

#include "std/condition_variable.hpp"
#include "std/function.hpp"
#include "std/mutex.hpp"
#include "std/unique_ptr.hpp"

/// Class, which performs any function when the specified
/// amount of time is elapsed.
class ScheduledTask
{
  typedef function<void()> fn_t;

  class Routine : public threads::IRoutine
  {
    fn_t m_fn;
    unsigned m_ms;

    condition_variable & m_condVar;
    mutex m_mutex;

  public:
    Routine(fn_t const & fn, unsigned ms, condition_variable & condVar);

    virtual void Do();
    virtual void Cancel();
  };

  /// The construction and destruction order is strict here: m_cond is
  /// used by routine that will be executed on m_thread.
  mutex m_mutex;
  condition_variable m_condVar;
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
