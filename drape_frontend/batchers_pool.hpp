#pragma once

#include "drape_frontend/tile_info.hpp"

#include "drape/pointers.hpp"
#include "drape/object_pool.hpp"
#include "drape/batcher.hpp"

#include "std/map.hpp"
#include "std/stack.hpp"
#include "std/function.hpp"

namespace df
{

class Message;
// Not thread safe
class BatchersPool
{
public:
  typedef function<void (drape_ptr<Message> &&)> TSendMessageFn;

  BatchersPool(int initBatcherCount, TSendMessageFn const & sendMessageFn);
  ~BatchersPool();

  void ReserveBatcher(TileKey const & key);
  ref_ptr<dp::Batcher> GetTileBatcher(TileKey const & key);
  void ReleaseBatcher(TileKey const & key);

private:
  typedef pair<dp::Batcher *, int> TBatcherPair;
  typedef map<TileKey, TBatcherPair> TBatcherMap;
  typedef TBatcherMap::iterator TIterator;
  TSendMessageFn m_sendMessageFn;

  ObjectPool<dp::Batcher, dp::BatcherFactory> m_pool;
  TBatcherMap m_batchs;
};

} // namespace df
