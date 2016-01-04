#include "drape_frontend/poi_symbol_shape.hpp"

#include "drape/utils/vertex_decl.hpp"
#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/glstate.hpp"
#include "drape/texture_manager.hpp"

#include "drape/shader_def.hpp"

namespace df
{

PoiSymbolShape::PoiSymbolShape(m2::PointF const & mercatorPt, PoiSymbolViewParams const & params)
  : m_pt(mercatorPt)
  , m_params(params)
{}

void PoiSymbolShape::Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const
{
  dp::TextureManager::SymbolRegion region;
  textures->GetSymbolRegion(m_params.m_symbolName, region);

  m2::PointU const pixelSize = region.GetPixelSize();
  m2::PointF const halfSize(pixelSize.x / 2.0, pixelSize.y / 2.0);
  m2::RectF const & texRect = region.GetTexRect();

  glsl::vec4 position = glsl::vec4(glsl::ToVec2(m_pt), m_params.m_depth, -m_params.m_posZ);

  gpu::SolidTexturingVertex vertexes[] =
  {
    gpu::SolidTexturingVertex{ position,
                               glsl::vec2(-halfSize.x, halfSize.y),
                               glsl::vec2(texRect.minX(), texRect.maxY())},
    gpu::SolidTexturingVertex{ position,
                               glsl::vec2(-halfSize.x, -halfSize.y),
                               glsl::vec2(texRect.minX(), texRect.minY())},
    gpu::SolidTexturingVertex{ position,
                               glsl::vec2(halfSize.x, halfSize.y),
                               glsl::vec2(texRect.maxX(), texRect.maxY())},
    gpu::SolidTexturingVertex{ position,
                               glsl::vec2(halfSize.x, -halfSize.y),
                               glsl::vec2(texRect.maxX(), texRect.minY())},
  };

  dp::GLState state(gpu::TEXTURING_PROGRAM, dp::GLState::OverlayLayer);
  state.SetProgram3dIndex(gpu::TEXTURING_BILLBOARD_PROGRAM);
  state.SetColorTexture(region.GetTexture());
  state.SetTextureFilter(gl_const::GLNearest);

  dp::AttributeProvider provider(1, 4);
  provider.InitStream(0, gpu::SolidTexturingVertex::GetBindingInfo(), make_ref(vertexes));

  drape_ptr<dp::OverlayHandle> handle = make_unique_dp<dp::SquareHandle>(m_params.m_id,
                                                                         dp::Center,
                                                                         m_pt, pixelSize,
                                                                         GetOverlayPriority(),
                                                                         true);
  handle->SetPivotZ(m_params.m_posZ);
  handle->SetExtendingSize(m_params.m_extendingSize);
  batcher->InsertTriangleStrip(state, make_ref(&provider), move(handle));
}

uint64_t PoiSymbolShape::GetOverlayPriority() const
{
  return dp::CalculateOverlayPriority(m_params.m_minVisibleScale, m_params.m_rank, m_params.m_depth);
}

} // namespace df
