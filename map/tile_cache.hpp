#pragma once

#include "tile.hpp"
#include "tiler.hpp"

#include "../yg/resource_manager.hpp"

#include "../base/mru_cache.hpp"
#include "../base/mutex.hpp"

#include "../std/bind.hpp"

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
        val.m_rm->renderTargetTextures()->Free(val.m_tile.m_renderTarget);
    }
  };

private:

  my::MRUCache<Tiler::RectInfo, Entry, EntryValueTraits> m_cache;
  threads::Mutex m_lock;

  TileCache(TileCache const & src);
  TileCache const & operator=(TileCache const & src);

public:

  TileCache(size_t maxCacheSize);
  /// lock for multithreaded access
  void lock();
  /// unlock for multithreaded access
  void unlock();
  /// get keys of values in cache
  set<Tiler::RectInfo> const & keys() const;
  /// add tile to cache
  void addTile(Tiler::RectInfo const & key, Entry const & entry);
  /// check, whether we have some tile in the cache
  bool hasTile(Tiler::RectInfo const & key);
  /// lock tile
  void lockTile(Tiler::RectInfo const & key);
  /// unlock tile
  void unlockTile(Tiler::RectInfo const & key);
  /// lock count
  size_t lockCount(Tiler::RectInfo const & key);
  /// touch tile
  void touchTile(Tiler::RectInfo const & key);
  /// get tile from the cache
  Tile const & getTile(Tiler::RectInfo const & key);
  /// remove the specified tile from the cache
  void remove(Tiler::RectInfo const & key);
};
