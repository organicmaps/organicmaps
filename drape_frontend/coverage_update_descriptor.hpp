#pragma once

#include "tile_info.hpp"

#include "../std/vector.hpp"

namespace df
{

class CoverageUpdateDescriptor
{
public:
  CoverageUpdateDescriptor();

  void DropAll();
  void DropTiles(TileKey const * tileToDrop, size_t size);


  bool IsDropAll() const;
  bool IsEmpty()   const;

  vector<TileKey> const & GetTilesToDrop() const;

private:
  bool m_doDropAll;
  vector<TileKey> m_tilesToDrop;
};

}


