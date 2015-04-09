#pragma once

#include "base/assert.hpp"
#include "base/mutex.hpp"

#include "std/list.hpp"
#include "std/set.hpp"

template <typename T, typename Factory>
class ObjectPool
{
private:
#ifdef DEBUG
  set<T *> m_checkerSet;
#endif
  list<T *> m_pool;
  Factory m_factory;
  threads::Mutex m_lock;
public:
  ObjectPool(int count, Factory const & f) : m_factory(f)
  {
    for (int i = 0; i < count; ++i)
    {
      T * novice = m_factory.GetNew();
#ifdef DEBUG
      m_checkerSet.insert(novice);
#endif
      m_pool.push_back(novice);
    }
  }

  ~ObjectPool()
  {
    for (typename list<T *>::iterator it = m_pool.begin(); it != m_pool.end(); it++)
    {
      T * cur = *it;
#ifdef DEBUG
      typename set<T *>::iterator its = m_checkerSet.find(cur);
      ASSERT(its != m_checkerSet.end(), ("The same element returned twice or more!"));
      m_checkerSet.erase(its);
#endif
      delete cur;
    }
#ifdef DEBUG
    ASSERT(m_checkerSet.empty(), ("Alert! Don't all elements returned to pool!"));
#endif
  }

  T * Get()
  {
    threads::MutexGuard guard(m_lock);
    if (m_pool.empty())
    {
      T * novice = m_factory.GetNew();
#ifdef DEBUG
      m_checkerSet.insert(novice);
#endif
      return novice;
    }
    else
    {
      T * pt = m_pool.front();
      m_pool.pop_front();
      return pt;
    }
  }

  void Return(T * object)
  {
    threads::MutexGuard guard(m_lock);
    m_pool.push_back(object);
  }
};

