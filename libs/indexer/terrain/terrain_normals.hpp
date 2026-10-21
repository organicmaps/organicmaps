#pragma once

#include "indexer/terrain/terrain_data.hpp"

#include "base/cancellable.hpp"

#include <vector>

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
};

// Unnormalized area-weighted vertex normals.
TerrainNormals CalculateTerrainNormals(TerrainData const & data, double zScale,
                                       base::SharedCancellable const & isCancellable = {});
}  // namespace terrain
