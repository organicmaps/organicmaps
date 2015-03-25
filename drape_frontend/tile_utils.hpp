#pragma once

#include "tile_key.hpp"
#include "../std/set.hpp"

namespace df
{

/// this function determines the coverage of a tile in specified zoom level
void CalcTilesCoverage(TileKey const & tileKey, int targetZoom, set<TileKey> & tiles);

/// this function determines the coverage of tiles in specified zoom level
void CalcTilesCoverage(set<TileKey> const & tileKeys, int targetZoom, set<TileKey> & tiles);

} // namespace df
