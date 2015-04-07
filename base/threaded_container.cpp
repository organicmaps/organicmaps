#include "base/threaded_container.hpp"

ThreadedContainer::ThreadedContainer() : m_WaitTime(0) {}

void ThreadedContainer::Cancel()
{
  threads::ConditionGuard g(m_Cond);
  my::Cancellable::Cancel();
  m_Cond.Signal(true);
}

double ThreadedContainer::WaitTime() const
{
  return m_WaitTime;
}
