#pragma once

#include "indexer/terrain/terrain_data.hpp"

#include "base/buffer_vector.hpp"
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
  buffer_vector<std::vector<TerrainNormal>, 8> m_raster;
};

// Unnormalized area-weighted normals. Grid cells contribute once; only their
// perimeter nodes are hashed to combine chunk and TIN-strip contributions.
TerrainNormals CalculateTerrainNormals(TerrainData const & data, double zScale,
                                       base::SharedCancellable const & isCancellable = {});
}  // namespace terrain
