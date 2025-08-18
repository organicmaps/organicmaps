#pragma once

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <list>
#include <mutex>
#include <set>
#include <string>

#define LOG_OBJECT_POOL

namespace dp
{
template <typename T, typename Factory>
class ObjectPool final
{
public:
  ObjectPool(int count, Factory const & f) : m_factory(f)
  {
    for (int i = 0; i < count; ++i)
    {
      T * novice = m_factory.GetNew();
#if defined(DEBUG) || defined(LOG_OBJECT_POOL)
      m_checkerSet.insert(novice);
#endif
      m_pool.push_back(novice);
    }
  }

  ~ObjectPool()
  {
    for (auto it = m_pool.begin(); it != m_pool.end(); it++)
    {
      T * cur = *it;
#if defined(DEBUG) || defined(LOG_OBJECT_POOL)
      auto its = m_checkerSet.find(cur);
      static std::string const kMessage = "The same element has been returned twice or more!";
      ASSERT(its != m_checkerSet.end(), (kMessage));
      if (its == m_checkerSet.end())
        LOG(LWARNING, (kMessage));
      m_checkerSet.erase(its);
#endif
      delete cur;
    }
#if defined(DEBUG) || defined(LOG_OBJECT_POOL)
    static std::string const kMessage2 = "Not all elements were returned to pool!";
    ASSERT(m_checkerSet.empty(), (kMessage2));
    if (!m_checkerSet.empty())
      LOG(LWARNING, (kMessage2));
#endif
  }

  T * Get()
  {
    std::lock_guard<std::mutex> lock(m_lock);
    if (m_pool.empty())
    {
      T * novice = m_factory.GetNew();
#if defined(DEBUG) || defined(LOG_OBJECT_POOL)
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
    std::lock_guard<std::mutex> lock(m_lock);
    m_pool.push_back(object);
  }

private:
#if defined(DEBUG) || defined(LOG_OBJECT_POOL)
  std::set<T *> m_checkerSet;
#endif
  std::list<T *> m_pool;
  Factory m_factory;
  std::mutex m_lock;
};
}  // namespace dp
