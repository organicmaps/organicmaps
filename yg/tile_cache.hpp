#pragma once

#include "tile.hpp"
#include "tiler.hpp"
#include "../base/cache.hpp"
#include "../base/mutex.hpp"

namespace yg
{
  class TileCache
  {
  private:

    my::Cache<uint64_t, Tile> m_cache;
    threads::Mutex m_mutex;

  public:

    TileCache(size_t tileMemSize, size_t memSize);
    /// lock for multithreaded access
    void lock();
    /// unlock for multithreaded access
    void unlock();
    /// add tile to cache
    void addTile(Tiler::RectInfo const & key, Tile const & value);
    /// check, whether we have some tile in the cache
    bool hasTile(Tiler::RectInfo const & key);
    /// get tile from the cache
    Tile const & getTile(Tiler::RectInfo const & key);
  };
}
