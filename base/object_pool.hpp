#pragma once

#include "condition.hpp"
#include "../std/list.hpp"

/// multithreaded object pool.
/// implements condition waiting scheme, to save the CPU cycles.
template <typename T>
class ObjectPool
{
private:

  list<T> m_List;

  threads::Condition m_Cond;

  bool m_IsCancelled;

public:

  ObjectPool() : m_IsCancelled(false)
  {}

  void Add(list<T> const & l)
  {
    for (typename list<T>::const_iterator it = l.begin(); it != l.end(); ++it)
      Free(*it);
  }

  size_t Size()
  {
    threads::ConditionGuard Guard(m_Cond);
    return m_List.size();
  }

  void Add(T const & t)
  {
    Free(t);
  }

  void Clear()
  {
    threads::ConditionGuard Guard(m_Cond);
    m_List.clear();
  }

  T const Reserve()
  {
    threads::ConditionGuard Guard(m_Cond);

    while (m_List.empty())
    {
      if (m_IsCancelled)
        break;
      m_Cond.Wait();
    }

    if (m_IsCancelled)
      return T();

    T res = m_List.front();
    m_List.pop_front();
    return res;
  }

  /// cancel all waiting requests
  void Cancel()
  {
    m_IsCancelled = true;
    m_Cond.Signal(true);
  }

  bool IsCancelled() const
  {
    return m_IsCancelled;
  }

  void Free(T const & t)
  {
    threads::ConditionGuard Guard(m_Cond);

    bool DoSignal = m_List.empty();

    m_List.push_back(t);

    if (DoSignal)
      m_Cond.Signal(); //< this doesn't release the mutex associated with m_cond,

    /// we should return as quickly as possible to minimize "waked threads" waiting time
  }
};
