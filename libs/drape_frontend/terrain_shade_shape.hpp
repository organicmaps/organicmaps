#pragma once

#include "drape_frontend/map_shape.hpp"

#include "drape/utils/vertex_decl.hpp"

#include <vector>

namespace df
{
// The terrain hillshade mesh of one tile: tile-local positions with the per-vertex
// Lambert intensity relative to the flat ground, smoothly interpolated by the
// TerrainShade program (see RuleDrawer::DrawTerrainShade).
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
