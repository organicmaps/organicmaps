#include "drape_frontend/area_shape.hpp"

#include "drape/shader_def.hpp"
#include "drape/glstate.hpp"
#include "drape/batcher.hpp"
#include "drape/attribute_provider.hpp"
#include "drape/texture_manager.hpp"
#include "drape/utils/vertex_decl.hpp"

#include "base/buffer_vector.hpp"
#include "base/logging.hpp"

#include "std/algorithm.hpp"

namespace df
{

AreaShape::AreaShape(vector<m2::PointF> && triangleList, AreaViewParams const & params)
  : m_vertexes(triangleList)
  , m_params(params)
{
}

void AreaShape::Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const
{
  dp::TextureManager::ColorRegion region;
  textures->GetColorRegion(m_params.m_color, region);
  glsl::vec2 const colorPoint = glsl::ToVec2(region.GetTexRect().Center());

  buffer_vector<gpu::SolidTexturingVertex, 128> vertexes;
  vertexes.resize(m_vertexes.size());
  transform(m_vertexes.begin(), m_vertexes.end(), vertexes.begin(), [&colorPoint, this](m2::PointF const & vertex)
  {
    return gpu::SolidTexturingVertex(glsl::vec3(glsl::ToVec2(vertex), m_params.m_depth),
                                     glsl::vec2(0.0, 0.0),
                                     colorPoint);
  });

  dp::GLState state(gpu::TEXTURING_PROGRAM, dp::GLState::GeometryLayer);
  state.SetColorTexture(region.GetTexture());

  dp::AttributeProvider provider(1, m_vertexes.size());
  provider.InitStream(0, gpu::SolidTexturingVertex::GetBindingInfo(), make_ref<void>(vertexes.data()));
  batcher->InsertTriangleList(state, make_ref(&provider));
}

} // namespace df
