#include "tile_cache.hpp"

namespace yg
{
  TileCache::Entry::Entry()
  {}

  TileCache::Entry::Entry(Tile const & tile, shared_ptr<yg::ResourceManager> const & rm)
    : m_tile(tile), m_rm(rm)
  {}


  TileCache::TileCache(size_t maxCacheSize)
    : m_cache(maxCacheSize)
  {
  }

  void TileCache::addTile(Tiler::RectInfo const & key, Entry const & entry)
  {
    m_cache.Add(key.toUInt64Cell(), entry, 1);
  }

  void TileCache::lock()
  {
    m_mutex.Lock();
  }

  void TileCache::unlock()
  {
    m_mutex.Unlock();
  }

  bool TileCache::hasTile(Tiler::RectInfo const & key)
  {
    return m_cache.HasElem(key.toUInt64Cell());
  }

  void TileCache::lockTile(Tiler::RectInfo const & key)
  {
    return m_cache.LockElem(key.toUInt64Cell());
  }

  void TileCache::unlockTile(Tiler::RectInfo const & key)
  {
    return m_cache.UnlockElem(key.toUInt64Cell());
  }

  void TileCache::touchTile(Tiler::RectInfo const & key)
  {
    m_cache.Touch(key.toUInt64Cell());
  }

  Tile const & TileCache::getTile(Tiler::RectInfo const & key)
  {
    return m_cache.Find(key.toUInt64Cell()).m_tile;
  }
}
