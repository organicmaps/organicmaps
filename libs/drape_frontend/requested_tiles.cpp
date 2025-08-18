#include "drape_frontend/requested_tiles.hpp"

namespace df
{
void RequestedTiles::Set(ScreenBase const & screen, bool have3dBuildings, bool forceRequest, bool forceUserMarksRequest,
                         TTilesCollection && tiles)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_tiles = std::move(tiles);
  m_screen = screen;
  m_have3dBuildings = have3dBuildings;
  m_forceRequest = forceRequest;
  m_forceUserMarksRequest = forceUserMarksRequest;
}

TTilesCollection RequestedTiles::GetTiles()
{
  TTilesCollection tiles;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_tiles.swap(tiles);
  }
  return tiles;
}

void RequestedTiles::GetParams(ScreenBase & screen, bool & have3dBuildings, bool & forceRequest,
                               bool & forceUserMarksRequest)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  screen = m_screen;
  have3dBuildings = m_have3dBuildings;
  forceRequest = m_forceRequest;
  forceUserMarksRequest = m_forceUserMarksRequest;
}

bool RequestedTiles::CheckTileKey(TileKey const & tileKey) const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_tiles.empty())
    return true;

  return m_tiles.find(tileKey) != m_tiles.end();
}
}  // namespace df
