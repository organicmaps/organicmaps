#include "drape_frontend/path_symbol_shape.hpp"
#include "drape_frontend/render_state_extension.hpp"
#include "drape_frontend/visual_params.hpp"

#include "shaders/programs.hpp"

#include "drape/utils/vertex_decl.hpp"
#include "drape/glsl_types.hpp"
#include "drape/glsl_func.hpp"
#include "drape/overlay_handle.hpp"
#include "drape/attribute_provider.hpp"
#include "drape/texture_manager.hpp"
#include "drape/batcher.hpp"

namespace df
{
PathSymbolShape::PathSymbolShape(m2::SharedSpline const & spline,
                                 PathSymbolViewParams const & params)
  : m_params(params)
  , m_spline(spline)
{}

void PathSymbolShape::Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const
{
  dp::TextureManager::SymbolRegion region;
  textures->GetSymbolRegion(m_params.m_symbolName, region);
  m2::RectF const & rect = region.GetTexRect();

  m2::PointF pixelSize = region.GetPixelSize();
  float halfW = 0.5f * pixelSize.x;
  float halfH = 0.5f * pixelSize.y;

  gpu::TSolidTexVertexBuffer buffer;

  m2::Spline::iterator splineIter = m_spline.CreateIterator();
  double pToGScale = 1.0 / m_params.m_baseGtoPScale;
  splineIter.Advance(m_params.m_offset * pToGScale);
  auto const step = static_cast<float>(m_params.m_step * pToGScale);
  glsl::vec2 dummy(0.0, 0.0);
  while (!splineIter.BeginAgain())
  {
    glsl::vec2 const pivot = glsl::ToVec2(ConvertToLocal(splineIter.m_pos, m_params.m_tileCenter, kShapeCoordScalar));
    glsl::vec2 n = halfH * glsl::normalize(glsl::vec2(-splineIter.m_dir.y, splineIter.m_dir.x));
    glsl::vec2 d = halfW * glsl::normalize(glsl::vec2(splineIter.m_dir.x, splineIter.m_dir.y));

    buffer.emplace_back(gpu::SolidTexturingVertex(glsl::vec4(pivot, m_params.m_depth, 0.0f), - d - n, glsl::ToVec2(rect.LeftTop())));
    buffer.emplace_back(gpu::SolidTexturingVertex(glsl::vec4(pivot, m_params.m_depth, 0.0f), - d + n, glsl::ToVec2(rect.LeftBottom())));
    buffer.emplace_back(gpu::SolidTexturingVertex(glsl::vec4(pivot, m_params.m_depth, 0.0f), d - n, glsl::ToVec2(rect.RightTop())));
    buffer.emplace_back(gpu::SolidTexturingVertex(glsl::vec4(pivot, m_params.m_depth, 0.0f), d + n, glsl::ToVec2(rect.RightBottom())));
    splineIter.Advance(step);
  }

  if (buffer.empty())
    return;

  auto state = CreateRenderState(gpu::Program::PathSymbol, DepthLayer::GeometryLayer);
  state.SetColorTexture(region.GetTexture());
  state.SetDepthTestEnabled(m_params.m_depthTestEnabled);

  dp::AttributeProvider provider(1, static_cast<uint32_t>(buffer.size()));
  provider.InitStream(0, gpu::SolidTexturingVertex::GetBindingInfo(), make_ref(buffer.data()));
  batcher->InsertListOfStrip(state, make_ref(&provider), 4);
}
}  // namespace df
