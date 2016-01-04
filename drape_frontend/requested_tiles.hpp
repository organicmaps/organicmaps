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
  void Set(ScreenBase const & screen, bool is3dBuildings, TTilesCollection && tiles);
  TTilesCollection GetTiles();
  ScreenBase GetScreen();
  bool Is3dBuildings();
  bool CheckTileKey(TileKey const & tileKey) const;

private:
  TTilesCollection m_tiles;
  ScreenBase m_screen;
  bool m_is3dBuildings = false;
  mutable mutex m_mutex;
};

} // namespace df
