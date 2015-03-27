#pragma once

#include "geometry/rect2d.hpp"

namespace df
{

enum TileStatus
{
  Unknown = 0,
  Rendered,
  Requested,
  Clipped
};

struct TileKey
{
  TileKey();
  TileKey(int x, int y, int zoomLevel);
  TileKey(int x, int y, int zoomLevel, TileStatus status);

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
