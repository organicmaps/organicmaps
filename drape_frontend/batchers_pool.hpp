#pragma once

#include "drape/batcher.hpp"
#include "drape/object_pool.hpp"

#include "base/assert.hpp"

#include <functional>
#include <map>
#include <utility>

namespace df
{
// Not thread safe
template <typename TKey, typename TKeyComparator>
class BatchersPool final
{
public:
  using TFlushFn =
      std::function<void(TKey const & key, dp::RenderState const & state, drape_ptr<dp::RenderBucket> && buffer)>;

  BatchersPool(int initBatchersCount, TFlushFn const & flushFn, uint32_t indexBufferSize, uint32_t vertexBufferSize)
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
    using namespace std::placeholders;
    m_batchers.insert(std::make_pair(key, std::make_pair(batcher, 1)));
    batcher->StartSession(std::bind(m_flushFn, key, _1, _2));
  }

  ref_ptr<dp::Batcher> GetBatcher(TKey const & key)
  {
    auto it = m_batchers.find(key);
    CHECK(it != m_batchers.end(), ());
    return make_ref(it->second.first);
  }

  void ReleaseBatcher(ref_ptr<dp::GraphicsContext> context, TKey const & key)
  {
    auto it = m_batchers.find(key);
    ASSERT(it != m_batchers.end(), ());
    ASSERT_GREATER(it->second.second, 0, ());
    if ((--it->second.second) == 0)
    {
      dp::Batcher * batcher = it->second.first;
      batcher->EndSession(context);
      m_pool.Return(batcher);
      m_batchers.erase(it);
    }
  }

private:
  using TBatcherPair = std::pair<dp::Batcher *, int>;
  using TBatcherMap = std::map<TKey, TBatcherPair, TKeyComparator>;
  TFlushFn m_flushFn;

  dp::ObjectPool<dp::Batcher, dp::BatcherFactory> m_pool;
  TBatcherMap m_batchers;
};
}  // namespace df
