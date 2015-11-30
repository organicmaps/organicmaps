#include "tile_utils.hpp"

#include "base/assert.hpp"
#include "base/stl_add.hpp"

namespace df
{

namespace
{

int Minificate(int coord, int zoom, int targetZoom)
{
  ASSERT(targetZoom < zoom, ());

  int z = zoom - targetZoom;
  if (coord >= 0)
    return coord >> z;

  // here we iteratively minificate zoom
  int c = -coord;
  ASSERT(c > 0, ());
  while (z > 0)
  {
    // c = c / 2 + 1, if c is odd
    // c = c / 2, if c is even
    c = (c + 1) >> 1;
    z--;
  }
  return -c;
}

} // namespace

void CalcTilesCoverage(TileKey const & tileKey, int targetZoom, TTilesCollection & tiles)
{
  CalcTilesCoverage(tileKey, targetZoom, MakeInsertFunctor(tiles));
}

void CalcTilesCoverage(set<TileKey> const & tileKeys, int targetZoom, TTilesCollection & tiles)
{
  for(TileKey const & tileKey : tileKeys)
    CalcTilesCoverage(tileKey, targetZoom, tiles);
}

void CalcTilesCoverage(TileKey const & tileKey, int targetZoom, function<void(TileKey const &)> const & processTile)
{
  ASSERT(processTile != nullptr, ());

  if (tileKey.m_zoomLevel == targetZoom)
    processTile(tileKey);
  else if (tileKey.m_zoomLevel > targetZoom)
  {
    // minification
    processTile(GetParentTile(tileKey, targetZoom));
  }
  else
  {
    // magnification
    int const z = targetZoom - tileKey.m_zoomLevel;
    int const tilesInRow = 1 << z;
    int const startX = tileKey.m_x << z;
    int const startY = tileKey.m_y << z;
    for (int x = 0; x < tilesInRow; x++)
      for (int y = 0; y < tilesInRow; y++)
        processTile(TileKey(startX + x, startY + y, targetZoom));
  }
}

bool IsTileAbove(TileKey const & tileKey, TileKey const & targetTileKey)
{
  if (tileKey.m_zoomLevel <= targetTileKey.m_zoomLevel)
    return false;

  int const x = Minificate(tileKey.m_x, tileKey.m_zoomLevel, targetTileKey.m_zoomLevel);
  if (x != targetTileKey.m_x)
    return false;

  int const y = Minificate(tileKey.m_y, tileKey.m_zoomLevel, targetTileKey.m_zoomLevel);
  return y == targetTileKey.m_y;
}

bool IsTileBelow(TileKey const & tileKey, TileKey const & targetTileKey)
{
  if (tileKey.m_zoomLevel >= targetTileKey.m_zoomLevel)
    return false;

  int const z = targetTileKey.m_zoomLevel - tileKey.m_zoomLevel;
  int const tilesInRow = 1 << z;
  int const startX = tileKey.m_x << z;
  if (targetTileKey.m_x < startX || targetTileKey.m_x >= startX + tilesInRow)
    return false;

  int const startY = tileKey.m_y << z;
  return targetTileKey.m_y >= startY && targetTileKey.m_y < startY + tilesInRow;
}

TileKey GetParentTile(TileKey const & tileKey, int targetZoom)
{
  ASSERT(tileKey.m_zoomLevel > targetZoom, ());

  return TileKey(Minificate(tileKey.m_x, tileKey.m_zoomLevel, targetZoom),
                 Minificate(tileKey.m_y, tileKey.m_zoomLevel, targetZoom),
                 targetZoom);
}

} // namespace df
