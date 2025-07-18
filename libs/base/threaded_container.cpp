#include "base/threaded_container.hpp"

void ThreadedContainer::Cancel()
{
  std::unique_lock lock(m_condLock);
  Cancellable::Cancel();
  m_Cond.notify_all();
}
