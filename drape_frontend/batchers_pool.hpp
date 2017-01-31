#pragma once

#include "drape/batcher.hpp"
#include "drape/object_pool.hpp"

#include "base/assert.hpp"

#include "std/bind.hpp"
#include "std/function.hpp"
#include "std/map.hpp"

namespace df
{

// Not thread safe
template <typename TKey, typename TKeyComparator>
class BatchersPool
{
public:
  using TFlushFn = function<void (TKey const & key, dp::GLState const & state, drape_ptr<dp::RenderBucket> && buffer)>;

  BatchersPool(int initBatchersCount, TFlushFn const & flushFn,
               uint32_t indexBufferSize, uint32_t vertexBufferSize)
    : m_flushFn(flushFn)
    , m_pool(initBatchersCount, dp::BatcherFactory(indexBufferSize, vertexBufferSize))
  {}

  ~BatchersPool()
  {
    for (auto const & p : m_batchers)
    {
      dp::Batcher * batcher = p.second.first;
      batcher->ResetSession();
      m_pool.Return(batcher);
    }

    m_batchers.clear();
  }

  void ReserveBatcher(TKey const & key)
  {
    auto it = m_batchers.find(key);
    if (it != m_batchers.end())
    {
      it->second.second++;
      return;
    }
    dp::Batcher * batcher = m_pool.Get();
    m_batchers.insert(make_pair(key, make_pair(batcher, 1)));
    batcher->StartSession(bind(m_flushFn, key, _1, _2));
  }

  ref_ptr<dp::Batcher> GetBatcher(TKey const & key)
  {
    auto it = m_batchers.find(key);
    ASSERT(it != m_batchers.end(), ());
    return make_ref(it->second.first);
  }

  void ReleaseBatcher(TKey const & key)
  {
    auto it = m_batchers.find(key);
    ASSERT(it != m_batchers.end(), ());
    ASSERT_GREATER(it->second.second, 0, ());
    if ((--it->second.second)== 0)
    {
      dp::Batcher * batcher = it->second.first;
      batcher->EndSession();
      m_pool.Return(batcher);
      m_batchers.erase(it);
    }
  }

private:
  using TBatcherPair = pair<dp::Batcher *, int>;
  using TBatcherMap = map<TKey, TBatcherPair, TKeyComparator>;
  TFlushFn m_flushFn;

  ObjectPool<dp::Batcher, dp::BatcherFactory> m_pool;
  TBatcherMap m_batchers;
};

} // namespace df
