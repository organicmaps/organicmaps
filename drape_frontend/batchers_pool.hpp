#pragma once

#include "drape/pointers.hpp"
#include "drape/object_pool.hpp"
#include "drape/batcher.hpp"

#include "base/assert.hpp"

#include "std/bind.hpp"
#include "std/map.hpp"
#include "std/stack.hpp"
#include "std/function.hpp"

namespace df
{

// Not thread safe
template <typename TKey, typename TKeyComparator>
class BatchersPool
{
public:
  typedef function<void (TKey const & key, dp::GLState const & state, drape_ptr<dp::RenderBucket> && buffer)> TFlushFn;

  BatchersPool(int initBatcherCount, TFlushFn const & flushFn)
    : m_flushFn(flushFn)
    , m_pool(initBatcherCount, dp::BatcherFactory())
  {}

  ~BatchersPool()
  {
    for_each(m_batchs.begin(), m_batchs.end(), [this](pair<TKey, TBatcherPair> const & p)
    {
      m_pool.Return(p.second.first);
    });

    m_batchs.clear();
  }

  void ReserveBatcher(TKey const & key)
  {
    TIterator it = m_batchs.find(key);
    if (it != m_batchs.end())
    {
      it->second.second++;
      return;
    }
    dp::Batcher * batcher = m_pool.Get();
    m_batchs.insert(make_pair(key, make_pair(batcher, 1)));
    batcher->StartSession(bind(m_flushFn, key, _1, _2));
  }

  ref_ptr<dp::Batcher> GetBatcher(TKey const & key)
  {
    TIterator it = m_batchs.find(key);
    ASSERT(it != m_batchs.end(), ());
    return make_ref(it->second.first);
  }

  void ReleaseBatcher(TKey const & key)
  {
    TIterator it = m_batchs.find(key);
    ASSERT(it != m_batchs.end(), ());
    ASSERT_GREATER(it->second.second, 0, ());
    if ((--it->second.second)== 0)
    {
      dp::Batcher * batcher = it->second.first;
      batcher->EndSession();
      m_pool.Return(batcher);
      m_batchs.erase(it);
    }
  }

private:
  typedef pair<dp::Batcher *, int> TBatcherPair;
  typedef map<TKey, TBatcherPair, TKeyComparator> TBatcherMap;
  typedef typename TBatcherMap::iterator TIterator;
  TFlushFn m_flushFn;

  ObjectPool<dp::Batcher, dp::BatcherFactory> m_pool;
  TBatcherMap m_batchs;
};

} // namespace df
