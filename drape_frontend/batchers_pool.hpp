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
  // Not thread safe
  class BatchersPool
  {
  public:
    typedef function<void (TransferPointer<Message>)> send_message_fn;

    BatchersPool(int initBatcherCount, const send_message_fn & sendMessageFn);
    ~BatchersPool();

    void ReserveBatcher(TileKey const & key);
    RefPointer<Batcher> GetTileBatcher(TileKey const & key);
    void ReleaseBatcher(TileKey const & key);

  private:
    typedef MasterPointer<Batcher> batcher_ptr;
    typedef stack<batcher_ptr> batchers_pool_t;
    typedef pair<batcher_ptr, int> counted_batcher_t;
    typedef map<TileKey, counted_batcher_t> reserved_batchers_t;

    batchers_pool_t m_batchers;
    reserved_batchers_t m_reservedBatchers;

    send_message_fn m_sendMessageFn;
  };
}
