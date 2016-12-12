#pragma once

#include "drape_frontend/tile_utils.hpp"

#include "geometry/screenbase.hpp"

#include "std/mutex.hpp"

namespace df
{

class RequestedTiles
{
public:
  RequestedTiles() = default;
  void Set(ScreenBase const & screen, bool have3dBuildings,
           bool needRegenerateTraffic, TTilesCollection && tiles);
  TTilesCollection GetTiles();
  void GetParams(ScreenBase & screen, bool & have3dBuildings,
                 bool & needRegenerateTraffic);
  bool CheckTileKey(TileKey const & tileKey) const;

private:
  TTilesCollection m_tiles;
  ScreenBase m_screen;
  bool m_have3dBuildings = false;
  bool m_needRegenerateTraffic = false;
  mutable mutex m_mutex;
};

} // namespace df
