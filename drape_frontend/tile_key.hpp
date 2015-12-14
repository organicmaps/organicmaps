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
  TileKey(TileKey const & key, uint64_t generation);

  bool operator < (TileKey const & other) const;
  bool operator == (TileKey const & other) const;

  m2::RectD GetGlobalRect(bool considerStyleZoom = false) const;

  int m_x;
  int m_y;
  int m_zoomLevel;
  int m_styleZoomLevel;
  uint64_t m_generation;
};

string DebugPrint(TileKey const & key);
string DebugPrint(TileStatus status);

} // namespace df
