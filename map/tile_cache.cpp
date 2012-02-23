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
  m_cache.Add(key, entry, 1);
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
  return m_cache.Keys();
/*  set<uint64_t> keys = m_cache.Keys();
  set<Tiler::RectInfo> rects;

  for (set<uint64_t>::const_iterator it = keys.begin(); it != keys.end(); ++it)
  {
    Tiler::RectInfo v;
    v.fromUInt64Cell(*it);
    rects.insert(v);
  }

  return rects;*/
}

bool TileCache::hasTile(Tiler::RectInfo const & key)
{
  return m_cache.HasElem(key);
}

void TileCache::lockTile(Tiler::RectInfo const & key)
{
  m_cache.LockElem(key);
}

size_t TileCache::lockCount(Tiler::RectInfo const & key)
{
  return m_cache.LockCount(key);
}

void TileCache::unlockTile(Tiler::RectInfo const & key)
{
  m_cache.UnlockElem(key);
}

void TileCache::touchTile(Tiler::RectInfo const & key)
{
  m_cache.Touch(key);
}

Tile const & TileCache::getTile(Tiler::RectInfo const & key)
{
  return m_cache.Find(key).m_tile;
}

void TileCache::remove(Tiler::RectInfo const & key)
{
  m_cache.Remove(key);
}
