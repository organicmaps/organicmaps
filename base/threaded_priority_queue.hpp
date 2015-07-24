#pragma once

#include "base/threaded_container.hpp"
#include "std/queue.hpp"

template <typename T>
class ThreadedPriorityQueue : public ThreadedContainer
{
private:
  priority_queue<T> m_queue;
public:

  template <typename Fn>
  void ProcessQueue(Fn const & fn)
  {
    threads::ConditionGuard g(m_Cond);

    bool hadElements = !m_queue.empty();

    fn(m_queue);

    bool hasElements = !m_queue.empty();

    if (!hadElements && hasElements)
      m_Cond.Signal();
  }

  void Push(T const & t)
  {
    threads::ConditionGuard g(m_Cond);

    bool doSignal = m_queue.empty();

    m_queue.push(t);

    if (doSignal)
      m_Cond.Signal();
  }

  bool WaitNonEmpty()
  {
    while (m_queue.empty())
    {
      if (IsCancelled())
        break;

      m_Cond.Wait();
    }

    if (IsCancelled())
      return true;

    return false;
  }

  T Top(bool doPop)
  {
    threads::ConditionGuard g(m_Cond);

    if (WaitNonEmpty())
      return T();

    T res = m_queue.top();

    if (doPop)
      m_queue.pop();

    return res;
  }

  bool Empty() const
  {
    threads::ConditionGuard g(m_Cond);
    return m_queue.empty();
  }

  void Clear()
  {
    threads::ConditionGuard g(m_Cond);
    while (!m_queue.empty())
      m_queue.pop();
  }
};
