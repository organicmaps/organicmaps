#include "tile_cache.hpp"

TileCache::Entry::Entry()
{}

TileCache::Entry::Entry(Tile const & tile, shared_ptr<yg::ResourceManager> const & rm)
  : m_tile(tile), m_rm(rm)
{}

TileCache::TileCache(size_t maxCacheSize)
  : m_cache(maxCacheSize)
{}

void TileCache::addTile(Tiler::RectInfo const & key, Entry const & entry)
{
  m_cache.Add(key.toUInt64Cell(), entry, 1);
}

void TileCache::readLock()
{
  m_lock.Lock();
}

void TileCache::readUnlock()
{
  m_lock.Unlock();
}

void TileCache::writeLock()
{
  m_lock.Lock();
}

void TileCache::writeUnlock()
{
  m_lock.Unlock();
}

set<Tiler::RectInfo> const TileCache::keys() const
{
  set<uint64_t> keys = m_cache.Keys();
  set<Tiler::RectInfo> rects;

  for (set<uint64_t>::const_iterator it = keys.begin(); it != keys.end(); ++it)
  {
    Tiler::RectInfo v;
    v.fromUInt64Cell(*it);
    rects.insert(v);
  }

  return rects;
}

bool TileCache::hasTile(Tiler::RectInfo const & key)
{
  return m_cache.HasElem(key.toUInt64Cell());
}

void TileCache::lockTile(Tiler::RectInfo const & key)
{
  m_cache.LockElem(key.toUInt64Cell());
}

size_t TileCache::lockCount(Tiler::RectInfo const & key)
{
  return m_cache.LockCount(key.toUInt64Cell());
}

void TileCache::unlockTile(Tiler::RectInfo const & key)
{
  m_cache.UnlockElem(key.toUInt64Cell());
}

void TileCache::touchTile(Tiler::RectInfo const & key)
{
  m_cache.Touch(key.toUInt64Cell());
}

Tile const & TileCache::getTile(Tiler::RectInfo const & key)
{
  return m_cache.Find(key.toUInt64Cell()).m_tile;
}
