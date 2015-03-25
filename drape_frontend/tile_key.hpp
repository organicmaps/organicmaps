#pragma once

#include "geometry/rect2d.hpp"

namespace df
{

struct TileKey
{
  TileKey();
  TileKey(int x, int y, int zoomLevel);

  bool operator < (TileKey const & other) const;
  bool operator == (TileKey const & other) const;

  m2::RectD GetGlobalRect() const;

  int m_x;
  int m_y;
  int m_zoomLevel;

private:
  friend string DebugPrint(TileKey const & );
};

} // namespace df
