#include "drape_frontend/batchers_pool.hpp"
#include "drape_frontend/message_subclasses.hpp"

#include "drape/batcher.hpp"

#include "base/assert.hpp"

#include "std/bind.hpp"

namespace df
{

using dp::Batcher;

namespace
{

void FlushGeometry(BatchersPool::TSendMessageFn const & sendMessage,
                   TileKey const & key,
                   dp::GLState const & state,
                   dp::TransferPointer<dp::RenderBucket> buffer)
{
  sendMessage(dp::MovePointer<Message>(new FlushRenderBucketMessage(key, state, buffer)));
}

} // namespace

BatchersPool::BatchersPool(int initBatcherCount, TSendMessageFn const & sendMessageFn)
  : m_sendMessageFn(sendMessageFn)
  , m_pool(initBatcherCount, dp::BatcherFactory())
{}

BatchersPool::~BatchersPool()
{
  for_each(m_batchs.begin(), m_batchs.end(), [this](pair<TileKey, TBatcherPair> const & p)
  {
    m_pool.Return(p.second.first);
  });

  m_batchs.clear();
}

void BatchersPool::ReserveBatcher(TileKey const & key)
{
  TIterator it = m_batchs.find(key);
  if (it != m_batchs.end())
  {
    it->second.second++;
    return;
  }
  Batcher * batcher = m_pool.Get();
  m_batchs.insert(make_pair(key, make_pair(batcher, 1)));
  batcher->StartSession(bind(&FlushGeometry, m_sendMessageFn, key, _1, _2));
}

dp::RefPointer<dp::Batcher> BatchersPool::GetTileBatcher(TileKey const & key)
{
  TIterator it = m_batchs.find(key);
  ASSERT(it != m_batchs.end(), ());
  return dp::MakeStackRefPointer(it->second.first);
}

void BatchersPool::ReleaseBatcher(TileKey const & key)
{
  TIterator it = m_batchs.find(key);
  ASSERT(it != m_batchs.end(), ());
  ASSERT_GREATER(it->second.second, 0, ());
  if ((--it->second.second)== 0)
  {
    Batcher * batcher = it->second.first;
    batcher->EndSession();
    m_pool.Return(batcher);
    m_batchs.erase(it);
  }
}

} // namespace df
