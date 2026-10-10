#pragma once

#include "indexer/terrain/terrain_data.hpp"

#include <functional>

namespace terrain
{
struct TerrainNormal
{
  double m_x = 0, m_y = 0, m_z = 0;
  TerrainNormal & operator+=(TerrainNormal const & other)
  {
    m_x += other.m_x;
    m_y += other.m_y;
    m_z += other.m_z;
    return *this;
  }
};

struct TerrainNormals
{
  std::vector<TerrainNormal> m_tin;
  std::vector<std::vector<TerrainNormal>> m_raster;
};

// Unnormalized area-weighted normals. Grid cells contribute once; only their
// perimeter nodes are hashed to combine chunk and TIN-strip contributions.
TerrainNormals CalculateTerrainNormals(TerrainData const & data, double zScale,
                                       std::function<bool()> const & isCancelled = {});
}  // namespace terrain
