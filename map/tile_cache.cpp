#include "tile_cache.hpp"

void TileCache::EntryValueTraits::Evict(Entry &val)
{
  if (val.m_rm)
    val.m_rm->texturePool(graphics::ERenderTargetTexture)->Free(val.m_tile.m_renderTarget);
}

TileCache::Entry::Entry()
{}

TileCache::Entry::Entry(Tile const & tile, shared_ptr<graphics::ResourceManager> const & rm)
  : m_tile(tile), m_rm(rm)
{}

TileCache::TileCache() : m_isLocked(false)
{}

void TileCache::AddTile(Tiler::RectInfo const & key, Entry const & entry)
{
  ASSERT(m_isLocked, ("TileCache need to be locked on modify"));
  m_cache.Add(key, entry, 1);
}

void TileCache::Lock()
{
  m_lock.Lock();
  m_isLocked = true;
}

void TileCache::Unlock()
{
  m_isLocked = false;
  m_lock.Unlock();
}

set<Tiler::RectInfo> const & TileCache::Keys() const
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

bool TileCache::HasTile(Tiler::RectInfo const & key)
{
  ASSERT(m_isLocked, ("TileCache need to be locked on modify"));
  return m_cache.HasElem(key);
}

void TileCache::LockTile(Tiler::RectInfo const & key)
{
  ASSERT(m_isLocked, ("TileCache need to be locked on modify"));
  m_cache.LockElem(key);
}

size_t TileCache::LockCount(Tiler::RectInfo const & key)
{
  ASSERT(m_isLocked, ("TileCache need to be locked on modify"));
  return m_cache.LockCount(key);
}

void TileCache::UnlockTile(Tiler::RectInfo const & key)
{
  ASSERT(m_isLocked, ("TileCache need to be locked on modify"));
  m_cache.UnlockElem(key);
}

void TileCache::TouchTile(Tiler::RectInfo const & key)
{
  ASSERT(m_isLocked, ("TileCache need to be locked on modify"));
  m_cache.Touch(key);
}

Tile const & TileCache::GetTile(Tiler::RectInfo const & key)
{
  ASSERT(m_isLocked, ("TileCache need to be locked on modify"));
  return m_cache.Find(key).m_tile;
}

void TileCache::Remove(Tiler::RectInfo const & key)
{
  ASSERT(m_isLocked, ("TileCache need to be locked on modify"));
  m_cache.Remove(key);
}

int TileCache::CanFit() const
{
  return m_cache.CanFit();
}

int TileCache::UnlockedWeight() const
{
  ASSERT(m_isLocked, ("TileCache need to be locked on modify"));
  return m_cache.UnlockedWeight();
}

int TileCache::LockedWeight() const
{
  ASSERT(m_isLocked, ("TileCache need to be locked on modify"));
  return m_cache.LockedWeight();
}

int TileCache::CacheSize() const
{
  return m_cache.MaxWeight();
}

void TileCache::Resize(int maxWeight)
{
  ASSERT(m_isLocked, ("TileCache need to be locked on modify"));
  m_cache.Resize(maxWeight);
}

void TileCache::FreeRoom(int weight)
{
  ASSERT(m_isLocked, ("TileCache need to be locked on modify"));
  m_cache.FreeRoom(weight);
}
