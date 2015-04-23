#include "drape_frontend/path_symbol_shape.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/utils/vertex_decl.hpp"
#include "drape/glsl_types.hpp"
#include "drape/glsl_func.hpp"
#include "drape/overlay_handle.hpp"
#include "drape/shader_def.hpp"
#include "drape/attribute_provider.hpp"
#include "drape/texture_manager.hpp"
#include "drape/glstate.hpp"
#include "drape/batcher.hpp"

namespace df
{

PathSymbolShape::PathSymbolShape(m2::SharedSpline const & spline,
                                 PathSymbolViewParams const & params)
  : m_params(params)
  , m_spline(spline)
{
}

void PathSymbolShape::Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const
{
  dp::TextureManager::SymbolRegion region;
  textures->GetSymbolRegion(m_params.m_symbolName, region);
  m2::RectF const & rect = region.GetTexRect();

  m2::PointU pixelSize = region.GetPixelSize();
  float halfW = pixelSize.x / 2.0f;
  float halfH = pixelSize.y / 2.0f;

  gpu::TSolidTexVertexBuffer buffer;

  m2::Spline::iterator splineIter = m_spline.CreateIterator();
  double pToGScale = 1.0 / m_params.m_baseGtoPScale;
  splineIter.Advance(m_params.m_offset * pToGScale);
  float step = m_params.m_step * pToGScale;
  glsl::vec2 dummy(0.0, 0.0);
  while (!splineIter.BeginAgain())
  {
    glsl::vec2 pivot = glsl::ToVec2(splineIter.m_pos);
    glsl::vec2 n = halfH * glsl::normalize(glsl::vec2(-splineIter.m_dir.y, splineIter.m_dir.x));
    glsl::vec2 d = halfW * glsl::normalize(glsl::vec2(splineIter.m_dir.x, splineIter.m_dir.y));
    float nLength = glsl::length(n) * pToGScale;
    float dLength = glsl::length(d) * pToGScale;
    n = nLength * glsl::normalize(n);
    d = dLength * glsl::normalize(d);

    buffer.push_back(gpu::SolidTexturingVertex(glsl::vec3(pivot - d - n, m_params.m_depth), dummy, glsl::ToVec2(rect.LeftTop())));
    buffer.push_back(gpu::SolidTexturingVertex(glsl::vec3(pivot - d + n, m_params.m_depth), dummy, glsl::ToVec2(rect.LeftBottom())));
    buffer.push_back(gpu::SolidTexturingVertex(glsl::vec3(pivot + d - n, m_params.m_depth), dummy, glsl::ToVec2(rect.RightTop())));
    buffer.push_back(gpu::SolidTexturingVertex(glsl::vec3(pivot + d + n, m_params.m_depth), dummy, glsl::ToVec2(rect.RightBottom())));
    splineIter.Advance(step);
  }

  if (buffer.empty())
    return;

  dp::GLState state(gpu::TEXTURING_PROGRAM, dp::GLState::GeometryLayer);
  state.SetColorTexture(region.GetTexture());

  dp::AttributeProvider provider(1, buffer.size());
  provider.InitStream(0, gpu::SolidTexturingVertex::GetBindingInfo(), make_ref(buffer.data()));
  batcher->InsertListOfStrip(state, make_ref(&provider), 4);
}

}
