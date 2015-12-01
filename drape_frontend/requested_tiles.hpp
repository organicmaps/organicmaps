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
  void Set(ScreenBase const & screen, TTilesCollection && tiles);
  TTilesCollection GetTiles();
  ScreenBase GetScreen();
  bool CheckTileKey(TileKey const & tileKey) const;

private:
  TTilesCollection m_tiles;
  ScreenBase m_screen;
  mutable mutex m_mutex;
};

} // namespace df
