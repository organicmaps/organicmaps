#include "drape_frontend/tile_utils.hpp"

#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

#include <cmath>

namespace df
{
CoverageResult CalcTilesCoverage(m2::RectD const & rect, int targetZoom,
                                 std::function<void(int, int)> const & processTile)
{
  ASSERT_GREATER(targetZoom, 0, ());
  double const rectSize = mercator::Bounds::kRangeX / (1 << (targetZoom - 1));

  CoverageResult result;
  result.m_minTileX = static_cast<int>(floor(rect.minX() / rectSize));
  result.m_maxTileX = static_cast<int>(ceil(rect.maxX() / rectSize));
  result.m_minTileY = static_cast<int>(floor(rect.minY() / rectSize));
  result.m_maxTileY = static_cast<int>(ceil(rect.maxY() / rectSize));

  if (processTile)
    result.ForEach(processTile);

  return result;
}

TTilesCollection CalcTilesToInvalidate(m2::RectD const & rect, m2::RectD const & clipRect, int zoom)
{
  TTilesCollection tiles;
  int const dataZoom = ClipTileZoomByMaxDataZoom(zoom);
  mercator::ForEachRectWrapped(rect, [&](m2::RectD const & wrappedRect)
  {
    CalcTilesCoverage(clipRect, dataZoom, [&](int tileX, int tileY)
    {
      TileKey const key(tileX, tileY, zoom);
      auto tileRect = key.GetGlobalRect();
      if (!tileRect.Intersect(clipRect))
        return;
      mercator::ForEachRectWrapped(tileRect, [&](m2::RectD const & wrappedTileRect)
      {
        auto intersection = wrappedRect;
        if (intersection.Intersect(wrappedTileRect) &&
            CalcTilesCoverage(intersection, dataZoom, {}).GetTilesCount() != 0)
          tiles.insert(key);
      });
    });
  });
  return tiles;
}

bool IsNeighbours(TileKey const & tileKey1, TileKey const & tileKey2)
{
  return !((tileKey1.m_x == tileKey2.m_x) && (tileKey1.m_y == tileKey2.m_y)) &&
         (abs(tileKey1.m_x - tileKey2.m_x) < 2) && (abs(tileKey1.m_y - tileKey2.m_y) < 2);
}

int ClipTileZoomByMaxDataZoom(int zoom)
{
  return std::min(zoom, scales::GetUpperScale());
}

TileKey GetTileKeyByPoint(m2::PointD const & pt, int zoom)
{
  ASSERT_GREATER(zoom, 0, ());
  double const rectSize = mercator::Bounds::kRangeX / (1 << (zoom - 1));
  return TileKey(static_cast<int>(floor(pt.x / rectSize)), static_cast<int>(floor(pt.y / rectSize)), zoom);
}
}  // namespace df
