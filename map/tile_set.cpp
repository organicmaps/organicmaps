#include "tile_set.hpp"

void TileSet::Lock()
{
  m_mutex.Lock();
}

void TileSet::Unlock()
{
  m_mutex.Unlock();
}

bool TileSet::HasTile(Tiler::RectInfo const & rectInfo)
{
  return m_tiles.find(rectInfo) != m_tiles.end();
}

void TileSet::AddTile(Tile const & tile)
{
  m_tiles[tile.m_rectInfo] = tile;
}

void TileSet::RemoveTile(const Tiler::RectInfo &rectInfo)
{
  m_tiles.erase(rectInfo);
}

Tile const & TileSet::GetTile(Tiler::RectInfo const & rectInfo)
{
  return m_tiles[rectInfo];
}

size_t TileSet::Size() const
{
  return m_tiles.size();
}
