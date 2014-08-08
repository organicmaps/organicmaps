#include "batchers_pool.hpp"
#include "message_subclasses.hpp"

#include "../drape/batcher.hpp"

#include "../base/assert.hpp"
#include "../base/logging.hpp"

#include "../std/bind.hpp"

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
  GLFunctions::glFlush();
  sendMessage(dp::MovePointer<Message>(new FlushRenderBucketMessage(key, state, buffer)));
}

} // namespace

BatchersPool::BatchersPool(int initBatcherCount, TSendMessageFn const & sendMessageFn)
  : m_sendMessageFn(sendMessageFn)
  , m_factory()
  , m_pool(initBatcherCount, m_factory)
{}

BatchersPool::~BatchersPool() {}

void BatchersPool::ReserveBatcher(TileKey const & key)
{
  TIterator it = m_batchs.find(key);
  if (it != m_batchs.end())
  {
    it->second.second++;
    return;
  }
  Batcher * B = m_pool.Get();
  m_batchs.insert(make_pair(key, make_pair(B, 1)));
  B->StartSession(bind(&FlushGeometry, m_sendMessageFn, key, _1, _2));
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
    Batcher * B = it->second.first;
    B->EndSession();
    m_pool.Return(B);
    m_batchs.erase(it);
  }
}

} // namespace df
