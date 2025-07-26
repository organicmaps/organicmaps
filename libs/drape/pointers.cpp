#include "drape/pointers.hpp"
#include "base/logging.hpp"

#if defined(TRACK_POINTERS)
DpPointerTracker & DpPointerTracker::Instance()
{
  static DpPointerTracker pointersTracker;
  return pointersTracker;
}

DpPointerTracker::~DpPointerTracker()
{
  ASSERT(m_alivePointers.empty(), ());
}

void DpPointerTracker::RefPtrNamed(void * refPtr, std::string const & name)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (refPtr != nullptr)
  {
    auto it = m_alivePointers.find(refPtr);
    if (it != m_alivePointers.end())
      it->second.first++;
    else
      m_alivePointers.insert(make_pair(refPtr, make_pair(1, name)));
  }
}

void DpPointerTracker::DestroyPtr(void * p)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  ASSERT(p != nullptr, ());
  auto it = m_alivePointers.find(p);
  if (it != m_alivePointers.end())
  {
    if (it->second.first != 0)
    {
      LOG(LWARNING, ("Drape pointer [", it->second.second, p,
                     "] was destroyed, but had references, ref count = ", it->second.first));
    }
    m_alivePointers.erase(it);
  }
}

void DpPointerTracker::DerefPtr(void * p)
{
  std::lock_guard<std::mutex> lock(m_mutex);
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

DpPointerTracker::TAlivePointers const & DpPointerTracker::GetAlivePointers() const
{
  return m_alivePointers;
}
#endif
