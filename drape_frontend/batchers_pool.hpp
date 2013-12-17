#pragma once

#include "tile_info.hpp"

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
    typedef function<void (Message *)> send_message_fn;

    BatchersPool(int initBatcherCount, const send_message_fn & sendMessageFn);
    ~BatchersPool();

    void AcceptMessage(Message * message);

  private:
    void ReserveBatcher(TileKey const & key);
    Batcher * GetTileBatcher(TileKey const & key);
    void ReleaseBatcher(TileKey const & key);

  private:
    stack<Batcher *> m_batchers;
    typedef map<TileKey, Batcher *> reserved_batchers_t;
    reserved_batchers_t m_reservedBatchers;

    send_message_fn m_sendMessageFn;
  };
}
