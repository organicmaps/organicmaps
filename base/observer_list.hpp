#pragma once

#include "base/logging.hpp"

#include <algorithm>
#include <mutex>
#include <vector>

namespace my
{
/// This class represents a thread-safe observers list.  It allows to
/// add/remove observers as well as trigger them all.
template <typename TObserver>
class ObserverList
{
public:
  bool Add(TObserver & observer)
  {
    std::lock_guard<std::mutex> lock(m_observersLock);
    auto const it = find(m_observers.begin(), m_observers.end(), &observer);
    if (it != m_observers.end())
    {
      LOG(LWARNING, ("Can't add the same observer twice:", &observer));
      return false;
    }
    m_observers.push_back(&observer);
    return true;
  }

  bool Remove(TObserver const & observer)
  {
    std::lock_guard<std::mutex> lock(m_observersLock);
    auto const it = find(m_observers.begin(), m_observers.end(), &observer);
    if (it == m_observers.end())
    {
      LOG(LWARNING, ("Can't remove non-registered observer:", &observer));
      return false;
    }
    m_observers.erase(it);
    return true;
  }

  template <typename F, typename... Args>
  void ForEach(F fn, Args const &... args)
  {
    std::lock_guard<std::mutex> lock(m_observersLock);
    for (TObserver * observer : m_observers)
      (observer->*fn)(args...);
  }

private:
  std::vector<TObserver *> m_observers;
  std::mutex m_observersLock;
};
}  // namespace my
