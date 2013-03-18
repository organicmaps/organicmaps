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

int TileSet::GetTileSequenceID(Tiler::RectInfo const & rectInfo)
{
  ASSERT(HasTile(rectInfo), ());
  return m_tiles[rectInfo].m_sequenceID;
}

void TileSet::SetTileSequenceID(Tiler::RectInfo const & rectInfo, int sequenceID)
{
  ASSERT(HasTile(rectInfo), ());
  m_tiles[rectInfo].m_sequenceID = sequenceID;
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
