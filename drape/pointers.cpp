#include "drape/pointers.hpp"
#include "base/logging.hpp"

DpPointerTracker & DpPointerTracker::Instance()
{
  static DpPointerTracker pointersTracker;
  return pointersTracker;
}

DpPointerTracker::~DpPointerTracker()
{
  ASSERT(m_alivePointers.empty(), ());
}

void DpPointerTracker::DestroyPtr(void * p)
{
  lock_guard<mutex> lock(m_mutex);
  ASSERT(p != nullptr, ());
  auto it = m_alivePointers.find(p);
  if (it != m_alivePointers.end())
  {
    if (it->second.first != 0)
    {
      LOG(LWARNING, ("Drape pointer [", it->second.second, p,
                     "] was destroyed, but had references, ref count = ",
                     it->second.first));
    }
    m_alivePointers.erase(it);
  }
}

void DpPointerTracker::DerefPtr(void * p)
{
  lock_guard<mutex> lock(m_mutex);
  if (p != nullptr)
  {
    auto it = m_alivePointers.find(p);
    if (it != m_alivePointers.end())
    {
      ASSERT(it->second.first > 0, ());
      it->second.first--;
    }
  }
}
