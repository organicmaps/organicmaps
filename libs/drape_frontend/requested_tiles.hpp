#pragma once

#include "drape_frontend/tile_utils.hpp"

#include "geometry/screenbase.hpp"

#include <mutex>

namespace df
{
class RequestedTiles
{
public:
  RequestedTiles() = default;
  void Set(ScreenBase const & screen, bool have3dBuildings, bool forceRequest, bool forceUserMarksRequest,
           TTilesCollection && tiles);
  TTilesCollection GetTiles();
  void GetParams(ScreenBase & screen, bool & have3dBuildings, bool & forceRequest, bool & forceUserMarksRequest);
  bool CheckTileKey(TileKey const & tileKey) const;

private:
  TTilesCollection m_tiles;
  ScreenBase m_screen;
  bool m_have3dBuildings = false;
  bool m_forceRequest = false;
  bool m_forceUserMarksRequest = false;
  mutable std::mutex m_mutex;
};
}  // namespace df
