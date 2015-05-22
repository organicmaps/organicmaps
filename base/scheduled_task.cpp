#include "base/scheduled_task.hpp"

#include "base/timer.hpp"
#include "base/logging.hpp"

#include "std/algorithm.hpp"
#include "std/mutex.hpp"

ScheduledTask::Routine::Routine(fn_t const & fn, milliseconds delay, atomic<bool> & started)
    : m_fn(fn), m_delay(delay), m_started(started)
{
}

void ScheduledTask::Routine::Do()
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
    m_fn();
  }
}

void ScheduledTask::Routine::Cancel()
{
  threads::IRoutine::Cancel();
  m_cv.notify_one();
}

ScheduledTask::ScheduledTask(fn_t const & fn, milliseconds ms) : m_started(false)
{
  m_thread.Create(make_unique<Routine>(fn, ms, m_started));
}

ScheduledTask::~ScheduledTask()
{
  CHECK(m_threadChecker.CalledOnOriginalThread(), ());
  m_thread.Cancel();
}

bool ScheduledTask::WasStarted() const
{
  CHECK(m_threadChecker.CalledOnOriginalThread(), ());
  return m_started;
}

void ScheduledTask::CancelNoBlocking()
{
  CHECK(m_threadChecker.CalledOnOriginalThread(), ());
  threads::IRoutine * routine = m_thread.GetRoutine();
  CHECK(routine, ());
  routine->Cancel();
}

void ScheduledTask::WaitForCompletion()
{
  CHECK(m_threadChecker.CalledOnOriginalThread(), ());
  m_thread.Join();
}
