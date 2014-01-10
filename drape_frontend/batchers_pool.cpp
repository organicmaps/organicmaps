#include "batchers_pool.hpp"
#include "message.hpp"
#include "message_subclasses.hpp"
#include "map_shape.hpp"

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
    ASSERT(m_reservedBatchers.empty(), ());
    while (!m_batchers.empty())
    {
      m_batchers.top().Destroy();
      m_batchers.pop();
    }
  }

  void BatchersPool::AcceptMessage(RefPointer<Message> message)
  {
    switch (message->GetType())
    {
    case Message::TileReadStarted:
      {
        BaseTileMessage * msg = static_cast<BaseTileMessage *>(message.GetRaw());
        ReserveBatcher(msg->GetKey());
        break;
      }
    case Message::TileReadEnded:
      {
        BaseTileMessage * msg = static_cast<BaseTileMessage *>(message.GetRaw());
        ReleaseBatcher(msg->GetKey());
        break;
      }
    case Message::MapShapeReaded:
      {
        MapShapeReadedMessage * mapShapeMessage = static_cast<MapShapeReadedMessage *>(message.GetRaw());
        RefPointer<Batcher> b = GetTileBatcher(mapShapeMessage->GetKey());
        MasterPointer<MapShape> shape(mapShapeMessage->GetShape());
        shape->Draw(b);
        shape.Destroy();
        break;
      }
    default:
      ASSERT(false, ());
      break;
    }
  }

  void BatchersPool::ReserveBatcher(TileKey const & key)
  {
    MasterPointer<Batcher> reserved;
    if (m_batchers.empty())
      reserved.Reset(new Batcher());
    else
    {
      reserved = m_batchers.top();
      m_batchers.pop();
    }

    reserved->StartSession(bind(&FlushGeometry, m_sendMessageFn, key, _1, _2));
    VERIFY(m_reservedBatchers.insert(make_pair(key, reserved)).second, ());
  }

  RefPointer<Batcher> BatchersPool::GetTileBatcher(TileKey const & key)
  {
    reserved_batchers_t::iterator it = m_reservedBatchers.find(key);

    ASSERT(it != m_reservedBatchers.end(), ());
    return it->second.GetRefPointer();
  }

  void BatchersPool::ReleaseBatcher(TileKey const & key)
  {
    reserved_batchers_t::iterator it = m_reservedBatchers.find(key);

    ASSERT(it != m_reservedBatchers.end(), ());
    MasterPointer<Batcher> batcher = it->second;
    batcher->EndSession();
    m_reservedBatchers.erase(it);
    m_batchers.push(batcher);
  }
}
