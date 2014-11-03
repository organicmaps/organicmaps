#include "area_shape.hpp"

#include "../drape/glsl_types.hpp"
#include "../drape/shader_def.hpp"
#include "../drape/glstate.hpp"
#include "../drape/batcher.hpp"
#include "../drape/attribute_provider.hpp"
#include "../drape/texture_set_holder.hpp"

#include "../base/buffer_vector.hpp"
#include "../base/logging.hpp"

#include "../std/algorithm.hpp"

namespace df
{

AreaShape::AreaShape(vector<m2::PointF> && triangleList, AreaViewParams const & params)
  : m_vertexes(triangleList)
  , m_params(params)
{
}

void AreaShape::Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const
{
  dp::ColorKey key(m_params.m_color.GetColorInInt());
  dp::TextureSetHolder::ColorRegion region;
  textures->GetColorRegion(key, region);
  m2::PointF const colorPoint = region.GetTexRect().Center();

  dp::TextureSetHolder::TextureNode const & texNode = region.GetTextureNode();

  buffer_vector<glsl::vec3, 128> colors;
  colors.resize(m_vertexes.size(), glsl::vec3(colorPoint.x, colorPoint.y, texNode.GetOffset()));

  buffer_vector<glsl::vec3, 128> geom;
  colors.reserve(m_vertexes.size());
  for_each(m_vertexes.begin(), m_vertexes.end(), [&geom, this] (m2::PointF const & vertex)
  {
    geom.push_back(glsl::vec3(vertex.x, vertex.y, m_params.m_depth));
  });

  dp::GLState state(gpu::SOLID_AREA_PROGRAM, dp::GLState::GeometryLayer);
  state.SetTextureSet(texNode.m_textureSet);

  dp::AttributeProvider provider(2, m_vertexes.size());
  {
    dp::BindingInfo info(1);
    dp::BindingDecl & decl = info.GetBindingDecl(0);
    decl.m_attributeName = "a_position";
    decl.m_componentCount = 3;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(0, info, dp::MakeStackRefPointer((void *)&geom[0]));
  }

  {
    dp::BindingInfo info(1);
    dp::BindingDecl & decl = info.GetBindingDecl(0);
    decl.m_attributeName = "a_color_index";
    decl.m_componentCount = 3;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(1, info, dp::MakeStackRefPointer((void *)&colors[0]));
  }

  batcher->InsertTriangleList(state, dp::MakeStackRefPointer(&provider));
}

} // namespace df
