#pragma once

#include "base/condition.hpp"
#include "base/macros.hpp"
#include "base/thread.hpp"

#include "std/chrono.hpp"
#include "std/condition_variable.hpp"
#include "std/function.hpp"
#include "std/unique_ptr.hpp"


#if defined(DEBUG) && !defined(OMIM_OS_ANDROID)
// This checker is not valid on Android.
// UI thread (NV thread) can change it's handle value during app lifecycle.
#define USE_THREAD_CHECKER
#endif

#ifdef USE_THREAD_CHECKER
#include "base/thread_checker.hpp"
#endif


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

#ifdef USE_THREAD_CHECKER
  ThreadChecker m_threadChecker;
#endif
  void CheckContext() const;

  DISALLOW_COPY_AND_MOVE(DeferredTask);
};
