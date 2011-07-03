#pragma once

#include "tile.hpp"
#include "tiler.hpp"
#include "../base/mru_cache.hpp"
#include "../base/mutex.hpp"

namespace yg
{
  class ResourceManager;

  class TileCache
  {
  public:

    struct Entry
    {
      Tile m_tile;
      shared_ptr<yg::ResourceManager> m_rm;
      Entry();
      Entry(Tile const & tile, shared_ptr<yg::ResourceManager> const & rm);
    };

    struct EntryValueTraits
    {
      static void Evict(Entry & val)
      {
        if (val.m_rm)
          val.m_rm->renderTargets().PushBack(val.m_tile.m_renderTarget);
      }
    };

  private:

    my::MRUCache<uint64_t, Entry, EntryValueTraits> m_cache;
    threads::Mutex m_mutex;

  public:

    TileCache(size_t maxCacheSize);
    /// lock for multithreaded access
    void lock();
    /// unlock for multithreaded access
    void unlock();
    /// add tile to cache
    void addTile(Tiler::RectInfo const & key, Entry const & entry);
    /// check, whether we have some tile in the cache
    bool hasTile(Tiler::RectInfo const & key);
    /// get tile from the cache
    Tile const & getTile(Tiler::RectInfo const & key);
  };
}
