#include "base/threaded_container.hpp"

void ThreadedContainer::Cancel()
{
  threads::ConditionGuard g(m_Cond);
  my::Cancellable::Cancel();
  m_Cond.Signal(true);

}
