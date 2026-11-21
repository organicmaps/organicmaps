#include "drape_frontend/path_symbol_shape.hpp"
#include "drape_frontend/line_shape_helper.hpp"
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

#include <algorithm>

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
  double const pixelToGlobalScale = 1.0 / m_params.m_baseGtoPScale;

  double const step = m_params.m_step * pixelToGlobalScale;
  ASSERT_GREATER(step, 0.0, ());
  auto iter = m_spline.CreateIterator();
  iter.Advance(m_params.m_offset * pixelToGlobalScale);
  for (; !iter.BeginAgain(); iter.Advance(step))
  {
    auto const & position = iter.m_pos;
    auto const & direction = iter.m_dir;
    auto segment = iter.GetIndex();
    glsl::vec4 const pivot(glsl::ToVec2(ConvertToLocal(position, m_params.m_tileCenter, kShapeCoordScalar)),
                           m_params.m_depth, 0.0f);

    auto tangent = direction;
    if (m_params.m_pointOffsetsPixels.size() == m_spline->GetPath().size())
    {
      auto const & path = m_spline->GetPath();
      segment = std::min(segment, path.size() - 2);
      auto const delta = m_params.m_pointOffsetsPixels[segment + 1] - m_params.m_pointOffsetsPixels[segment];
      tangent = path[segment + 1] - path[segment] + m2::PointD(delta.x, delta.y) * pixelToGlobalScale;
      if (tangent.SquaredLength() == 0)
        tangent = direction;
    }
    glsl::vec2 const n = halfPxSize.y * glsl::normalize(glsl::vec2(-tangent.y, tangent.x));
    glsl::vec2 const d = halfPxSize.x * glsl::normalize(glsl::vec2(tangent.x, tangent.y));
    glsl::vec2 offset = m_params.m_lateralOffsetPixels * glsl::normalize(glsl::vec2(direction.y, -direction.x));
    if (m_params.m_pointOffsetsPixels.size() == m_spline->GetPath().size())
      offset = InterpolateLinePixelOffset(m_spline->GetPath(), m_params.m_pointOffsetsPixels, segment, position);


    buffer.emplace_back(pivot, offset - d - n, lt);
    buffer.emplace_back(pivot, offset - d + n, lb);
    buffer.emplace_back(pivot, offset + d - n, rt);
    buffer.emplace_back(pivot, offset + d + n, rb);
  }

  if (buffer.empty())
    return;

  auto state = CreateRenderState(gpu::Program::PathSymbol, m_params.m_depthLayer);
  state.SetColorTexture(region.GetTexture());
  state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
  state.SetTextureIndex(region.GetTextureIndex());

  dp::AttributeProvider provider(1, static_cast<uint32_t>(buffer.size()));
  provider.InitStream(0, gpu::SolidTexturingVertex::GetBindingInfo(), make_ref(buffer.data()));
  batcher->InsertListOfStrip(context, state, make_ref(&provider), 4);
}
}  // namespace df
