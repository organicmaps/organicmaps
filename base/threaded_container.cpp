#include "base/threaded_container.hpp"

void ThreadedContainer::Cancel()
{
  threads::ConditionGuard g(m_Cond);
  base::Cancellable::Cancel();
  m_Cond.Signal(true);
}
