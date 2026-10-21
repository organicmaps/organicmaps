#pragma once

#include "indexer/terrain/tile_mesh.hpp"

namespace terrain
{
struct TerrainData
{
  explicit TerrainData(uint8_t coordBits = kTerrainCoordBits) : m_mesh(coordBits) {}

  TileMesh m_mesh;

  bool IsEmpty() const { return m_mesh.IsEmpty(); }
};
}  // namespace terrain
