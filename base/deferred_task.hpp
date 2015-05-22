#pragma once

#include "base/condition.hpp"
#include "base/macros.hpp"
#include "base/thread.hpp"
#include "base/thread_checker.hpp"

#include "std/chrono.hpp"
#include "std/condition_variable.hpp"
#include "std/function.hpp"
#include "std/unique_ptr.hpp"

// This class is used to call a function after waiting for a specified
// amount of time.  The function is called in a separate thread.  This
// class is not thread safe.
class DeferredTask
{
public:
  typedef function<void()> TTask;

  DeferredTask(TTask const & task, milliseconds ms);

  ~DeferredTask();

  /// Returns true if task was started after delay.
  bool WasStarted() const;

  /// Cancels task without waiting for worker thread termination.
  void Cancel();

  /// Waits for task's completion and worker thread termination.
  void WaitForCompletion();

private:
  class Routine : public threads::IRoutine
  {
    TTask const m_task;
    milliseconds const m_delay;
    condition_variable m_cv;
    atomic<bool> & m_started;

  public:
    Routine(TTask const & task, milliseconds delay, atomic<bool> & started);

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

  DISALLOW_COPY_AND_MOVE(DeferredTask);
};
