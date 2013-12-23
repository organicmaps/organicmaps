#pragma once

#include "tile_info.hpp"

#include "../drape/pointers.hpp"

#include "../std/map.hpp"
#include "../std/stack.hpp"
#include "../std/function.hpp"

class Batcher;

namespace df
{
  class Message;
  class BatchersPool
  {
  public:
    typedef function<void (TransferPointer<Message>)> send_message_fn;

    BatchersPool(int initBatcherCount, const send_message_fn & sendMessageFn);
    ~BatchersPool();

    void AcceptMessage(RefPointer<Message> message);

  private:
    void ReserveBatcher(TileKey const & key);
    RefPointer<Batcher> GetTileBatcher(TileKey const & key);
    void ReleaseBatcher(TileKey const & key);

  private:
    stack<MasterPointer<Batcher> > m_batchers;
    typedef map<TileKey, MasterPointer<Batcher> > reserved_batchers_t;
    reserved_batchers_t m_reservedBatchers;

    send_message_fn m_sendMessageFn;
  };
}
