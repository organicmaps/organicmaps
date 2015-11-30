#pragma once

#include "geometry/rect2d.hpp"

namespace df
{

enum class TileStatus
{
  // tile does not participate in rendering or fake
  Unknown = 0,
  // tile is rendered
  Rendered,
  // tile has been requested to be rendered
  Requested,
  // tile is ready but it was deferred for rendering
  Deferred
};

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
};

string DebugPrint(TileKey const & key);
string DebugPrint(TileStatus status);

} // namespace df
