#pragma once

#include "tile.hpp"
#include "tiler.hpp"

#include "../yg/resource_manager.hpp"

#include "../base/mru_cache.hpp"
#include "../base/mutex.hpp"

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
  threads::Mutex m_lock;

public:

  TileCache(size_t maxCacheSize);
  /// lock for multithreaded READ access
  void readLock();
  /// unlock for multithreaded READ access
  void readUnlock();
  /// lock for multithreaded WRITE access
  void writeLock();
  /// unlock for multithreaded WRITE access
  void writeUnlock();
  /// add tile to cache
  void addTile(Tiler::RectInfo const & key, Entry const & entry);
  /// check, whether we have some tile in the cache
  bool hasTile(Tiler::RectInfo const & key);
  /// lock tile
  void lockTile(Tiler::RectInfo const & key);
  /// unlock tile
  void unlockTile(Tiler::RectInfo const & key);
  /// touch tile
  void touchTile(Tiler::RectInfo const & key);
  /// get tile from the cache
  Tile const & getTile(Tiler::RectInfo const & key);
};
