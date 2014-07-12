#include "pointers.hpp"

PointerTracker::~PointerTracker()
{
  ASSERT(m_countMap.empty(), ());
}

void PointerTracker::Deref(void * p)
{
  threads::MutexGuard g(m_mutex);
  if (p == NULL)
    return;

  map_t::iterator it = m_countMap.find(p);
  ASSERT(it != m_countMap.end(), ());
  ASSERT(it->second.first > 0, ());

  if (--it->second.first == 0)
  {
    ASSERT(m_alivePointers.find(p) == m_alivePointers.end(), ("Pointer leak for type : ", it->second.second));
    m_countMap.erase(it);
  }
}

void PointerTracker::Destroy(void * p)
{
  threads::MutexGuard g(m_mutex);
  if (p == NULL)
    return;

  map_t::iterator it = m_countMap.find(p);
  if (it == m_countMap.end()) // suppress warning in release build
    ASSERT(false, ());

  ASSERT(it->second.first == 1, ("Delete memory with more than one user : ", it->second.second));
  ASSERT(m_alivePointers.erase(p) == 1, ());
}

#if defined(CHECK_POINTERS)
  PointerTracker g_tracker;
#endif
