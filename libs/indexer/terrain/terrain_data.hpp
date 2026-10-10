#pragma once

#include "indexer/terrain/raster_patch.hpp"

namespace terrain
{
struct TerrainData
{
  explicit TerrainData(uint8_t coordBits = kTerrainCoordBits) : m_mesh(coordBits) {}

  TileMesh m_mesh;
  std::vector<RasterPatch> m_rasterPatches;

  bool IsEmpty() const { return m_mesh.IsEmpty() && m_rasterPatches.empty(); }
  void Append(TerrainData && other);
};
}  // namespace terrain
