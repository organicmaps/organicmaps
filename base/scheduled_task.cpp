#include "scheduled_task.hpp"

ScheduledTask::Routine::Routine(fn_t const & fn, size_t ms)
  : m_Fn(fn), m_Interval(ms)
{}

void ScheduledTask::Routine::Do()
{
  m_Cond.Lock();
  m_Cond.Wait(m_Interval);
  if (!IsCancelled())
    m_Fn();
  m_Cond.Unlock();
}

ScheduledTask::ScheduledTask(fn_t const & fn, size_t ms)
{
  m_Thread.Create(new Routine(fn, ms));
}

void ScheduledTask::Cancel()
{
  m_Thread.Cancel();
}
