#pragma once

#include "base/threaded_container.hpp"

#include <atomic>
#include <list>

template <typename T>
class ThreadedList : public ThreadedContainer
{
private:

  std::list<T> m_list;
  std::atomic<bool> m_isEmpty;

  bool WaitNonEmpty(std::unique_lock<std::mutex> & lock)
  {
    /// @todo Remove unused Empty(), m_isEmpty, pass stop predicate into wait.
    while ((m_isEmpty = m_list.empty()))
    {
      if (IsCancelled())
        break;

      m_Cond.wait(lock);
    }

    return IsCancelled();
  }
public:

  ThreadedList()
    : m_isEmpty(true)
  {}

  template <typename Fn>
  void ProcessList(Fn const & fn)
  {
    std::unique_lock<std::mutex> lock(m_condLock);

    bool hadElements = !m_list.empty();

    fn(m_list);

    bool hasElements = !m_list.empty();

    m_isEmpty = !hasElements;

    if (!hadElements && hasElements)
      m_Cond.notify_all();
  }

  void PushBack(T const & t)
  {
    std::unique_lock<std::mutex> lock(m_condLock);

    bool doSignal = m_list.empty();

    m_list.push_back(t);
    m_isEmpty = false;

    if (doSignal)
      m_Cond.notify_all();
  }

  void PushFront(T const & t)
  {
    std::unique_lock<std::mutex> lock(m_condLock);

    bool doSignal = m_list.empty();

    m_list.push_front(t);
    m_isEmpty = false;

    if (doSignal)
      m_Cond.notify_all();
  }

  T const Front(bool doPop)
  {
    std::unique_lock<std::mutex> lock(m_condLock);

    if (WaitNonEmpty(lock))
      return T();

    T res = m_list.front();

    if (doPop)
      m_list.pop_front();

    m_isEmpty = m_list.empty();

    return res;
  }

  T const Back(bool doPop)
  {
    std::unique_lock<std::mutex> lock(m_condLock);

    if (WaitNonEmpty(lock))
      return T();

    T res = m_list.back();

    if (doPop)
      m_list.pop_back();

    m_isEmpty = m_list.empty();

    return res;
  }

  size_t Size() const
  {
    std::unique_lock<std::mutex> lock(m_condLock);
    return m_list.size();
  }

  bool Empty() const
  {
    return m_isEmpty;
  }

  void Clear()
  {
    std::unique_lock<std::mutex> lock(m_condLock);
    m_list.clear();
    m_isEmpty = true;
  }
};
