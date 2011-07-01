#include "tile_cache.hpp"

#include "../std/cmath.hpp"


namespace yg
{
  TileCache::TileCache(size_t tileMemSize, size_t memSize)
    : m_cache(log(memSize / static_cast<double>(tileMemSize)) / log(2.0))
  {
  }

  void TileCache::addTile(Tiler::RectInfo const & key, Tile const & value)
  {
    bool found;
    Tile & cachedVal = m_cache.Find(key.toUInt64Cell(), found);
    cachedVal = value;
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
    return m_cache.HasKey(key.toUInt64Cell());
  }

  Tile const & TileCache::getTile(Tiler::RectInfo const & key)
  {
    bool found;
    return m_cache.Find(key.toUInt64Cell(), found);
  }
}
