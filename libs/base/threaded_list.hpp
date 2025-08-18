#pragma once

#include "base/threaded_container.hpp"

#include <list>

template <typename T>
class ThreadedList : public ThreadedContainer
{
private:
  std::list<T> m_list;

  /// @return true if ThreadedContainer::Cancel was called.
  bool WaitNonEmptyOrCancel(std::unique_lock<std::mutex> & lock)
  {
    m_Cond.wait(lock, [this]() { return !m_list.empty() || IsCancelled(); });
    return IsCancelled();
  }

public:
  template <typename Fn>
  void ProcessList(Fn const & fn)
  {
    std::unique_lock lock(m_condLock);

    bool const hadElements = !m_list.empty();

    fn(m_list);

    bool const hasElements = !m_list.empty();

    if (!hadElements && hasElements)
      m_Cond.notify_all();
  }

  void PushBack(T const & t)
  {
    std::unique_lock lock(m_condLock);

    bool const doSignal = m_list.empty();

    m_list.push_back(t);

    if (doSignal)
      m_Cond.notify_all();
  }

  void PushFront(T const & t)
  {
    std::unique_lock lock(m_condLock);

    bool const doSignal = m_list.empty();

    m_list.push_front(t);

    if (doSignal)
      m_Cond.notify_all();
  }

  T Front(bool doPop)
  {
    std::unique_lock lock(m_condLock);

    if (WaitNonEmptyOrCancel(lock))
      return T();

    T res = m_list.front();

    if (doPop)
      m_list.pop_front();

    return res;
  }

  T Back(bool doPop)
  {
    std::unique_lock lock(m_condLock);

    if (WaitNonEmpty(lock))
      return T();

    T res = m_list.back();

    if (doPop)
      m_list.pop_back();

    return res;
  }

  size_t Size() const
  {
    std::unique_lock lock(m_condLock);
    return m_list.size();
  }

  void Clear()
  {
    std::unique_lock lock(m_condLock);
    m_list.clear();
  }
};
