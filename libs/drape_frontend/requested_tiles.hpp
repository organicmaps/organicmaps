#pragma once

#include "drape_frontend/tile_utils.hpp"

#include "geometry/screenbase.hpp"

#include <mutex>

namespace df
{
class RequestedTiles
{
public:
  explicit RequestedTiles(bool trackRetiredTiles = false) : m_trackRetiredTiles(trackRetiredTiles) {}
  void Set(ScreenBase const & screen, bool have3dBuildings, bool forceRequest, bool forceUserMarksRequest,
           TTilesCollection && tiles);
  TTilesCollection Get(ScreenBase & screen, bool & have3dBuildings, bool & forceRequest, bool & forceUserMarksRequest);
  bool CheckTileKey(TileKey const & tileKey) const;

private:
  TTilesCollection m_tiles;
  TTilesCollection m_lastTiles, m_retiredTiles;
  bool const m_trackRetiredTiles;
  ScreenBase m_screen;
  bool m_have3dBuildings = false;
  bool m_forceRequest = false;
  bool m_forceUserMarksRequest = false;
  mutable std::mutex m_mutex;
};
}  // namespace df
