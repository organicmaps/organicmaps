#pragma once

#include "drape_frontend/map_shape.hpp"

#include "drape/utils/vertex_decl.hpp"

#include "indexer/terrain/terrain_data.hpp"

#include "geometry/rect2d.hpp"

#include "base/cancellable.hpp"

#include <vector>

namespace df
{
// The tile hillshade in tile-local coordinates: the TIN triangles clipped to the tile as the CW
// triangles (see TerrainShadeShape). Empty when cancelled.
struct TerrainShadeGeometry
{
  std::vector<gpu::TerrainShadeVertex> m_triangles;
};

TerrainShadeGeometry BuildTerrainShade(terrain::TerrainData const & data, m2::RectD const & tileRect,
                                       base::SharedCancellable const & cancellable);

// Tile-local terrain positions and surface normals. TerrainShade computes lighting
// per vertex from the live light direction and interpolates the resulting intensity.
class TerrainShadeShape : public MapShape
{
public:
  explicit TerrainShadeShape(std::vector<gpu::TerrainShadeVertex> && vertices) : m_vertices(std::move(vertices)) {}

  void Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
            ref_ptr<dp::TextureManager> textures) const override;

private:
  std::vector<gpu::TerrainShadeVertex> m_vertices;
};
}  // namespace df
