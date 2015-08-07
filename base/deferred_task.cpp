#include "base/deferred_task.hpp"

#include "base/timer.hpp"
#include "base/logging.hpp"

#include "std/algorithm.hpp"
#include "std/mutex.hpp"

DeferredTask::Routine::Routine(TTask const & task, milliseconds delay, atomic<bool> & started)
    : m_task(task), m_delay(delay), m_started(started)
{
}

void DeferredTask::Routine::Do()
{
  mutex mu;
  unique_lock<mutex> lock(mu);

  steady_clock::time_point const end = steady_clock::now() + m_delay;
  while (!IsCancelled())
  {
    steady_clock::time_point const current = steady_clock::now();
    if (current >= end)
      break;
    m_cv.wait_for(lock, end - current, [this]()
    {
      return IsCancelled();
    });
  }

  if (!IsCancelled())
  {
    m_started = true;
    m_task();
  }
}

void DeferredTask::Routine::Cancel()
{
  threads::IRoutine::Cancel();
  m_cv.notify_one();
}

DeferredTask::DeferredTask(TTask const & task, milliseconds ms) : m_started(false)
{
  m_thread.Create(make_unique<Routine>(task, ms, m_started));
}

DeferredTask::~DeferredTask()
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  m_thread.Cancel();
}

bool DeferredTask::WasStarted() const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  return m_started;
}

void DeferredTask::Cancel()
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  threads::IRoutine * routine = m_thread.GetRoutine();
  CHECK(routine, ());
  routine->Cancel();
}

void DeferredTask::WaitForCompletion()
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  m_thread.Join();
}
