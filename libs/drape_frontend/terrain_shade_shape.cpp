#include "drape_frontend/terrain_shade_shape.hpp"

#include "drape_frontend/render_state_extension.hpp"

#include "shaders/programs.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"

namespace df
{
void TerrainShadeShape::Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
                             ref_ptr<dp::TextureManager> /* textures */) const
{
  // The geometry layer with the default alpha blending, drawn after every area fill and
  // pattern in the program order (see the Program enum). The intensity is a vertex
  // attribute, so no color texture is needed.
  auto state = CreateRenderState(gpu::Program::TerrainShade, DepthLayer::GeometryLayer);
  state.SetDepthTestEnabled(true);

  dp::AttributeProvider provider(1, static_cast<uint32_t>(m_vertices.size()));
  // The provider only reads the stream; ref_ptr is not const-aware.
  provider.InitStream(0, gpu::TerrainShadeVertex::GetBindingInfo(),
                      make_ref(const_cast<gpu::TerrainShadeVertex *>(m_vertices.data())));
  batcher->InsertTriangleList(context, state, make_ref(&provider));
}
}  // namespace df
