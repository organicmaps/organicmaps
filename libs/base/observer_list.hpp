#pragma once

#include "base/logging.hpp"

#include <algorithm>
#include <mutex>
#include <utility>
#include <vector>

namespace base
{
class DummyLockable
{
public:
  void lock() {}
  void unlock() {}
};

template <typename Observer, typename Mutex>
class ObserverList;

/// This alias represents a thread-safe observers list.  It allows to
/// add/remove observers as well as trigger them all.
template <typename Observer>
using ObserverListSafe = ObserverList<Observer, std::mutex>;

template <typename Observer>
using ObserverListUnsafe = ObserverList<Observer, DummyLockable>;

template <typename Observer, typename Mutex>
class ObserverList
{
public:
  bool Add(Observer & observer)
  {
    std::lock_guard<Mutex> lock(m_observersLock);
    auto const it = std::find(m_observers.begin(), m_observers.end(), &observer);
    if (it != m_observers.end())
    {
      LOG(LWARNING, ("Can't add the same observer twice:", &observer));
      return false;
    }
    m_observers.push_back(&observer);
    return true;
  }

  bool Remove(Observer const & observer)
  {
    std::lock_guard<Mutex> lock(m_observersLock);
    auto const it = std::find(m_observers.begin(), m_observers.end(), &observer);
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
    std::lock_guard<Mutex> lock(m_observersLock);
    for (Observer * observer : m_observers)
      (observer->*fn)(args...);
  }

private:
  Mutex m_observersLock;
  std::vector<Observer *> m_observers;
};
}  // namespace base
