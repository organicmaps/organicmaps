#include "base/scheduled_task.hpp"
#include "base/timer.hpp"

#include "../base/logging.hpp"

#include "../std/algorithm.hpp"
#include "../std/chrono.hpp"

ScheduledTask::Routine::Routine(fn_t const & fn,
                                unsigned ms,
                                condition_variable & condVar)
  : m_fn(fn),
    m_ms(ms),
    m_condVar(condVar)
{}

void ScheduledTask::Routine::Do()
{
  unique_lock<mutex> lock(m_mutex);

  milliseconds timeLeft(m_ms);
  while (!IsCancelled() && timeLeft != milliseconds::zero())
  {
    my::Timer t;
    m_condVar.wait_for(lock, timeLeft, [this]()
                       {
                         return IsCancelled();
                       });
    milliseconds timeElapsed(static_cast<unsigned>(t.ElapsedSeconds() * 1000));
    timeLeft -= min(timeLeft, timeElapsed);
  }

  if (!IsCancelled())
    m_fn();
}

void ScheduledTask::Routine::Cancel()
{
  threads::IRoutine::Cancel();
  m_condVar.notify_one();
}

ScheduledTask::ScheduledTask(fn_t const & fn, unsigned ms)
{
  m_thread.Create(make_unique<Routine>(fn, ms, m_condVar));
}

ScheduledTask::~ScheduledTask()
{
  CancelBlocking();
}

bool ScheduledTask::CancelNoBlocking()
{
  if (!m_thread.GetRoutine())
    return false;
  m_thread.GetRoutine()->Cancel();
  return true;
}

void ScheduledTask::CancelBlocking()
{
  m_thread.Cancel();
}
