#pragma once

#include "../std/list.hpp"
#include "../std/vector.hpp"
#include "../std/set.hpp"

#include "../base/assert.hpp"

#include "../../drape_frontend/read_mwm_task.hpp"

template <typename T, typename Factory>
class ObjectPool
{
private:
#ifdef DEBUG
  std::set<T*> checkerSet;
#endif
  std::list<T*> pool;
  Factory factory;
public:
  ObjectPool(int count, Factory const & f) : factory(f)
  {
#ifdef DEBUG
    for(int i = 0; i < count; ++i)
    {
      T * novice = factory.GetNew();
      checkerSet.insert(novice);
      pool.push_back(novice);
    }
#else
    for(int i = 0; i < count; ++i)
      pool.push_back(factory.GetNew());
#endif
  }

  ~ObjectPool()
  {
    for (typename list<T*>::iterator it = pool.begin(); it != pool.end(); it++)
    {
      T * cur = *it;
#ifdef DEBUG
      typename set<T*>::iterator its = checkerSet.find(cur);
      ASSERT(its != checkerSet.end(), ("The same element returned twice or more!"));
      checkerSet.erase(its);
#endif
      delete cur;
    }
#ifdef DEBUG
    ASSERT(checkerSet.empty(), ("Alert! Don't all elements returned to pool!"));
#endif
  }

  T * Get()
  {
    if (pool.empty())
    {
#ifdef DEBUG
      T * novice = factory.GetNew();
      checkerSet.insert(novice);
      return novice;
#else
      return factory.GetNew();
#endif
    }
    else
    {
      T* pt = pool.front();
      pool.pop_front();
      return pt;
    }
  }

  void Return(T * object)
  {
    object->Reset();
    pool.push_back(object);
  }
};

