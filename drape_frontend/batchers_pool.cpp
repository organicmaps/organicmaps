#include "batchers_pool.hpp"
#include "message_subclasses.hpp"

#include "../drape/batcher.hpp"

#include "../base/assert.hpp"

#include "../std/bind.hpp"

namespace df
{

namespace
{

void FlushGeometry(BatchersPool::send_message_fn const & sendMessage,
                   TileKey const & key,
                   dp::GLState const & state,
                   dp::TransferPointer<dp::RenderBucket> buffer)
{
  GLFunctions::glFlush();
  sendMessage(dp::MovePointer<Message>(new FlushRenderBucketMessage(key, state, buffer)));
}

} // namespace

BatchersPool::BatchersPool(int initBatcherCount, send_message_fn const & sendMessageFn)
  : m_sendMessageFn(sendMessageFn)
{
  for (int i = 0; i < initBatcherCount; ++i)
    m_batchers.push(dp::MasterPointer<dp::Batcher>(new dp::Batcher()));
}

BatchersPool::~BatchersPool()
{
  for (reserved_batchers_t::iterator it = m_reservedBatchers.begin();
       it != m_reservedBatchers.end(); ++it)
  {
    it->second.first.Destroy();
  }
  m_reservedBatchers.clear();

  while (!m_batchers.empty())
  {
    m_batchers.top().Destroy();
    m_batchers.pop();
  }
}

void BatchersPool::ReserveBatcher(TileKey const & key)
{
  reserved_batchers_t::iterator it = m_reservedBatchers.find(key);
  if (it != m_reservedBatchers.end())
  {
    it->second.second++;
    return;
  }

  dp::MasterPointer<dp::Batcher> reserved;
  if (m_batchers.empty())
    reserved.Reset(new dp::Batcher());
  else
  {
    reserved = m_batchers.top();
    m_batchers.pop();
  }

  reserved->StartSession(bind(&FlushGeometry, m_sendMessageFn, key, _1, _2));
  VERIFY(m_reservedBatchers.insert(make_pair(key, make_pair(reserved, 1))).second, ());
}

dp::RefPointer<dp::Batcher> BatchersPool::GetTileBatcher(TileKey const & key)
{
  reserved_batchers_t::iterator it = m_reservedBatchers.find(key);

  ASSERT(it != m_reservedBatchers.end(), ());
  return it->second.first.GetRefPointer();
}

void BatchersPool::ReleaseBatcher(TileKey const & key)
{
  reserved_batchers_t::iterator it = m_reservedBatchers.find(key);

  ASSERT(it != m_reservedBatchers.end(), ());
  ASSERT_GREATER(it->second.second, 0, ());
  if ((--it->second.second)== 0)
  {
    dp::MasterPointer<dp::Batcher> batcher = it->second.first;
    batcher->EndSession();
    m_reservedBatchers.erase(it);
    m_batchers.push(batcher);
  }
}

} // namespace df
