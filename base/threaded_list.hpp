#pragma once

#include "condition.hpp"
#include "../std/list.hpp"

#include "threaded_container.hpp"

template <typename T>
class ThreadedList : public ThreadedContainer
{
private:

  list<T> m_list;

public:

  template <typename Fn>
  void ProcessList(Fn const & fn)
  {
    threads::ConditionGuard g(m_Cond);

    bool hadElements = !m_list.empty();

    fn(m_list);

    bool hasElements = !m_list.empty();

    if (!hadElements && hasElements)
      m_Cond.Signal();
  }

  void PushBack(T const & t)
  {
    threads::ConditionGuard g(m_Cond);

    bool doSignal = m_list.empty();

    m_list.push_back(t);

    if (doSignal)
      m_Cond.Signal();
  }

  void PushFront(T const & t)
  {
    threads::ConditionGuard g(m_Cond);

    bool doSignal = m_list.empty();

    m_list.push_front(t);

    if (doSignal)
      m_Cond.Signal();
  }

  bool WaitNonEmpty()
  {
    double StartWaitTime = m_Timer.ElapsedSeconds();

    while (m_list.empty())
    {
      if (IsCancelled())
        break;
      m_Cond.Wait();
    }

    m_WaitTime += m_Timer.ElapsedSeconds() - StartWaitTime;

    if (IsCancelled())
      return true;

    return false;
  }

  T const Front(bool doPop)
  {
    threads::ConditionGuard g(m_Cond);

    if (WaitNonEmpty())
      return T();

    T res = m_list.front();

    if (doPop)
      m_list.pop_front();

    return res;
  }

  T const Back(bool doPop)
  {
    threads::ConditionGuard g(m_Cond);

    if (WaitNonEmpty())
      return T();

    T res = m_list.back();

    if (doPop)
      m_list.pop_back();

    return res;
  }

  size_t Size()
  {
    threads::ConditionGuard g(m_Cond);
    return m_list.size();
  }

  bool Empty()
  {
    threads::ConditionGuard g(m_Cond);
    return m_list.empty();
  }

  void Clear()
  {
    threads::ConditionGuard g(m_Cond);
    m_list.clear();
  }
};
