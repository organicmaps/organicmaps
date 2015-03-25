#include <tile_utils.hpp>

#include "../base/assert.hpp"
#include "../std/cmath.hpp"

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
  int c = abs(coord);
  while (z > 0)
  {
    c = (c >> 1) + ((c % 2) != 0 ? 1 : 0);
    z--;
  }
  return -c;
}

}

void CalcTilesCoverage(TileKey const & tileKey, int targetZoom, set<TileKey> & tiles)
{
  if (tileKey.m_zoomLevel == targetZoom)
    tiles.emplace(tileKey);
  else if (tileKey.m_zoomLevel > targetZoom)
  {
    // minification
    tiles.emplace(Minificate(tileKey.m_x, tileKey.m_zoomLevel, targetZoom),
                  Minificate(tileKey.m_y, tileKey.m_zoomLevel, targetZoom),
                  targetZoom);
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
        tiles.emplace(startX + x, startY + y, targetZoom);
  }
}

void CalcTilesCoverage(set<TileKey> const & tileKeys, int targetZoom, set<TileKey> & tiles)
{
  for(TileKey const & tileKey : tileKeys)
    CalcTilesCoverage(tileKey, targetZoom, tiles);
}

} // namespace df
