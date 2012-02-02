#include "threaded_container.hpp"

ThreadedContainer::ThreadedContainer()
  : m_WaitTime(0), m_IsCancelled(false)
{
}

void ThreadedContainer::Cancel()
{
  threads::ConditionGuard g(m_Cond);
  m_IsCancelled = true;
  m_Cond.Signal(true);
}

bool ThreadedContainer::IsCancelled() const
{
  return m_IsCancelled;
}

double ThreadedContainer::WaitTime() const
{
  return m_WaitTime;
}
