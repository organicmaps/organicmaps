#include "pointers.hpp"

PointerTracker::~PointerTracker()
{
  ASSERT(m_countMap.empty(), ());
}

void PointerTracker::Deref(void * p, bool needDestroyedCheck)
{
  if (p == NULL)
    return;

  map_t::iterator it = m_countMap.find(p);
  ASSERT(it != m_countMap.end(), ());
  ASSERT(it->second.first > 0, ());

  if (--it->second.first == 0)
  {
    if (needDestroyedCheck)
      ASSERT(m_alivePointers.find(p) == m_alivePointers.end(), ("Pointer leak for type : ", it->second.second));
    m_countMap.erase(it);
  }
}

void PointerTracker::Destroy(void * p)
{
  if (p == NULL)
    return;

  map_t::iterator it = m_countMap.find(p);
  if (it == m_countMap.end()) // suppress warning in release build
    ASSERT(false, ());

  ASSERT(it->second.first == 1, ("Delete memory with more than one user : ", it->second.second));
  ASSERT(m_alivePointers.erase(p) == 1, ());
}

#ifdef DEBUG
PointerTracker g_tracker;
#endif
