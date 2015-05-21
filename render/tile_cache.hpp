#pragma once

#include "tile.hpp"
#include "tiler.hpp"

#include "graphics/resource_manager.hpp"

#include "base/mru_cache.hpp"
#include "base/mutex.hpp"

#include "std/bind.hpp"

namespace graphics
{
  class ResourceManager;
}

class TileCache
{
public:

  struct Entry
  {
    Tile m_tile;
    shared_ptr<graphics::ResourceManager> m_rm;
    Entry();
    Entry(Tile const & tile, shared_ptr<graphics::ResourceManager> const & rm);
  };

  struct EntryValueTraits
  {
    static void Evict(Entry & val);
  };

private:

  my::MRUCache<Tiler::RectInfo, Entry, EntryValueTraits> m_cache;
  threads::Mutex m_lock;
  bool m_isLocked;

  TileCache(TileCache const & src);
  TileCache const & operator=(TileCache const & src);

public:

  TileCache();
  /// lock for multithreaded access
  void Lock();
  /// unlock for multithreaded access
  void Unlock();
  /// get keys of values in cache
  set<Tiler::RectInfo> const & Keys() const;
  /// add tile to cache
  void AddTile(Tiler::RectInfo const & key, Entry const & entry);
  /// check, whether we have some tile in the cache
  bool HasTile(Tiler::RectInfo const & key);
  /// lock tile
  void LockTile(Tiler::RectInfo const & key);
  /// unlock tile
  void UnlockTile(Tiler::RectInfo const & key);
  /// lock count
  size_t LockCount(Tiler::RectInfo const & key);
  /// touch tile
  void TouchTile(Tiler::RectInfo const & key);
  /// get tile from the cache
  Tile const & GetTile(Tiler::RectInfo const & key);
  /// remove the specified tile from the cache
  void Remove(Tiler::RectInfo const & key);
  /// how much elements can fit in the tileCache
  int CanFit() const;
  /// how many unlocked elements do we have in tileCache
  int UnlockedWeight() const;
  /// how many locked elements do we have in tileCache
  int LockedWeight() const;
  /// the size of the cache
  int CacheSize() const;
  /// resize the cache
  void Resize(int maxWeight);
  /// free up to weight spaces evicting unlocked elements from cache
  void FreeRoom(int weight);
};
