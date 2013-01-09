#include "scheduled_task.hpp"
#include "timer.hpp"

ScheduledTask::Routine::Routine(fn_t const & fn,
                                unsigned ms,
                                threads::Condition * cond)
  : m_fn(fn),
    m_ms(ms),
    m_pCond(cond)
{}

void ScheduledTask::Routine::Do()
{
  m_pCond->Lock();

  unsigned msLeft = m_ms;

  while (!IsCancelled())
  {
    my::Timer t;

    if (m_pCond->Wait(msLeft))
      break;

    msLeft -= (unsigned)(t.ElapsedSeconds() * 1000);
  }

  if (!IsCancelled())
    m_fn();

  m_pCond->Unlock();
}

void ScheduledTask::Routine::Cancel()
{
  m_pCond->Lock();
  IRoutine::Cancel();
  m_pCond->Signal();
  m_pCond->Unlock();
}

ScheduledTask::ScheduledTask(fn_t const & fn, unsigned ms)
{
  m_thread.Create(new Routine(fn, ms, &m_cond));
}

void ScheduledTask::Cancel()
{
  m_thread.Cancel();
}
