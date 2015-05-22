#pragma once

#include "base/condition.hpp"
#include "base/thread.hpp"
#include "base/thread_checker.hpp"

#include "std/chrono.hpp"
#include "std/condition_variable.hpp"
#include "std/function.hpp"
#include "std/unique_ptr.hpp"

/// Class, which performs any function when the specified amount of
/// time is elapsed. This class is not thread safe.
class ScheduledTask
{
  typedef function<void()> fn_t;

  class Routine : public threads::IRoutine
  {
    fn_t const m_fn;
    milliseconds const m_delay;
    condition_variable m_cv;
    atomic<bool> & m_started;

  public:
    Routine(fn_t const & fn, milliseconds delay, atomic<bool> & started);

    // IRoutine overrides:
    void Do() override;

    // my::Cancellable overrides:
    void Cancel() override;
  };

  /// The construction and destruction order is strict here: m_started
  /// is used by routine that will be executed on m_thread.
  atomic<bool> m_started;
  threads::Thread m_thread;

  ThreadChecker m_threadChecker;

public:
  ScheduledTask(fn_t const & fn, milliseconds ms);

  ~ScheduledTask();

  /// Returns true if task was started after delay.
  bool WasStarted() const;

  /// Cancels task without waiting for worker thread termination.
  void CancelNoBlocking();

  /// Waits for task's completion and worker thread termination.
  void WaitForCompletion();
};
