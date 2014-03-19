#include "batchers_pool.hpp"
#include "message_subclasses.hpp"

#include "../drape/batcher.hpp"

#include "../base/assert.hpp"

#include "../std/bind.hpp"

namespace df
{
  namespace
  {
    void FlushGeometry(const BatchersPool::send_message_fn & sendMessage,
                       const TileKey & key,
                       const GLState & state,
                       TransferPointer<VertexArrayBuffer> buffer)
    {
      GLFunctions::glFlush();
      sendMessage(MovePointer<Message>(new FlushTileMessage(key, state, buffer)));
    }
  }

  BatchersPool::BatchersPool(int initBatcherCount, const send_message_fn & sendMessageFn)
    : m_sendMessageFn(sendMessageFn)
  {
    for (int i = 0; i < initBatcherCount; ++i)
      m_batchers.push(MasterPointer<Batcher>(new Batcher()));
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

    MasterPointer<Batcher> reserved;
    if (m_batchers.empty())
      reserved.Reset(new Batcher());
    else
    {
      reserved = m_batchers.top();
      m_batchers.pop();
    }

    reserved->StartSession(bind(&FlushGeometry, m_sendMessageFn, key, _1, _2));
    VERIFY(m_reservedBatchers.insert(make_pair(key, make_pair(reserved, 1))).second, ());
  }

  RefPointer<Batcher> BatchersPool::GetTileBatcher(TileKey const & key)
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
      MasterPointer<Batcher> batcher = it->second.first;
      batcher->EndSession();
      m_reservedBatchers.erase(it);
      m_batchers.push(batcher);
    }
  }
}
