#pragma once

#include "drape_frontend/tile_key.hpp"

#include <functional>
#include <set>

namespace df
{
using TTilesCollection = std::set<TileKey>;

struct CoverageResult
{
  int m_minTileX = 0;
  int m_maxTileX = 0;
  int m_minTileY = 0;
  int m_maxTileY = 0;

  bool IsOneTile() const { return m_minTileX + 1 == m_maxTileX && m_minTileY + 1 == m_maxTileY; }
  size_t GetTilesCount() const { return size_t(m_maxTileX - m_minTileX) * size_t(m_maxTileY - m_minTileY); }

  template <class FnT>
  void ForEach(FnT && fn) const
  {
    for (int tileY = m_minTileY; tileY < m_maxTileY; ++tileY)
      for (int tileX = m_minTileX; tileX < m_maxTileX; ++tileX)
        fn(tileX, tileY);
  }
};

// This function determines the tiles coverage in specified zoom level.
// Each tile can be processed in processTile callback.
CoverageResult CalcTilesCoverage(m2::RectD const & rect, int targetZoom,
                                 std::function<void(int, int)> const & processTile);

// This function checks if tileKey1 and tileKey2 are neighbours
bool IsNeighbours(TileKey const & tileKey1, TileKey const & tileKey2);

// This function performs clipping by maximum zoom label available for map data.
int ClipTileZoomByMaxDataZoom(int zoom);

// This function returns tile key by point on specific zoom level.
TileKey GetTileKeyByPoint(m2::PointD const & pt, int zoom);
}  // namespace df
