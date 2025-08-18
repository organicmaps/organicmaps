#include "drape_frontend/path_symbol_shape.hpp"
#include "drape_frontend/render_state_extension.hpp"
#include "drape_frontend/visual_params.hpp"

#include "shaders/programs.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/glsl_func.hpp"
#include "drape/glsl_types.hpp"
#include "drape/overlay_handle.hpp"
#include "drape/texture_manager.hpp"
#include "drape/utils/vertex_decl.hpp"

namespace df
{
PathSymbolShape::PathSymbolShape(m2::SharedSpline const & spline, PathSymbolViewParams const & params)
  : m_params(params)
  , m_spline(spline)
{}

void PathSymbolShape::Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
                           ref_ptr<dp::TextureManager> textures) const
{
  dp::TextureManager::SymbolRegion region;
  textures->GetSymbolRegion(m_params.m_symbolName, region);
  m2::RectF const & rect = region.GetTexRect();
  auto const lt = glsl::ToVec2(rect.LeftTop());
  auto const lb = glsl::ToVec2(rect.LeftBottom());
  auto const rt = glsl::ToVec2(rect.RightTop());
  auto const rb = glsl::ToVec2(rect.RightBottom());

  m2::PointF const halfPxSize = region.GetPixelSize() * 0.5f;

  gpu::VBUnknownSizeT<gpu::SolidTexturingVertex> buffer;

  m2::Spline::iterator splineIter = m_spline.CreateIterator();
  double pToGScale = 1.0 / m_params.m_baseGtoPScale;
  splineIter.Advance(m_params.m_offset * pToGScale);
  auto const step = static_cast<float>(m_params.m_step * pToGScale);

  while (!splineIter.BeginAgain())
  {
    glsl::vec4 const pivot(glsl::ToVec2(ConvertToLocal(splineIter.m_pos, m_params.m_tileCenter, kShapeCoordScalar)),
                           m_params.m_depth, 0.0f);

    glsl::vec2 const n = halfPxSize.y * glsl::normalize(glsl::vec2(-splineIter.m_dir.y, splineIter.m_dir.x));
    glsl::vec2 const d = halfPxSize.x * glsl::normalize(glsl::vec2(splineIter.m_dir.x, splineIter.m_dir.y));

    buffer.emplace_back(pivot, -d - n, lt);
    buffer.emplace_back(pivot, -d + n, lb);
    buffer.emplace_back(pivot, d - n, rt);
    buffer.emplace_back(pivot, d + n, rb);
    splineIter.Advance(step);
  }

  if (buffer.empty())
    return;

  auto state = CreateRenderState(gpu::Program::PathSymbol, DepthLayer::GeometryLayer);
  state.SetColorTexture(region.GetTexture());
  state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
  state.SetTextureIndex(region.GetTextureIndex());

  dp::AttributeProvider provider(1, static_cast<uint32_t>(buffer.size()));
  provider.InitStream(0, gpu::SolidTexturingVertex::GetBindingInfo(), make_ref(buffer.data()));
  batcher->InsertListOfStrip(context, state, make_ref(&provider), 4);
}
}  // namespace df
