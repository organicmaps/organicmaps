#include "path_symbol_shape.hpp"
#include "visual_params.hpp"

#include "../drape/glsl_types.hpp"
#include "../drape/glsl_func.hpp"
#include "../drape/overlay_handle.hpp"
#include "../drape/shader_def.hpp"
#include "../drape/attribute_provider.hpp"
#include "../drape/glstate.hpp"
#include "../drape/batcher.hpp"
#include "../drape/texture_set_holder.hpp"

namespace df
{

int const VertexPerQuad = 4;

PathSymbolShape::PathSymbolShape(m2::SharedSpline const & spline,
                                 PathSymbolViewParams const & params)
  : m_params(params)
  , m_spline(spline)
{
}

void PathSymbolShape::Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const
{
  buffer_vector<glsl::Quad3, 128> positions;
  buffer_vector<glsl::Quad2, 128> normals;
  buffer_vector<glsl::Quad3, 128> texCoord;

  dp::TextureSetHolder::SymbolRegion region;
  textures->GetSymbolRegion(m_params.m_symbolName, region);
  m2::PointU pixelSize;
  region.GetPixelSize(pixelSize);
  double pToGScale = 1.0 / m_params.m_baseGtoPScale;
  double halfW = pixelSize.x / 2.0f;
  double halfH = pixelSize.y / 2.0f;

  m2::Spline::iterator splineIter = m_spline.CreateIterator();
  splineIter.Step(m_params.m_offset * pToGScale);
  float step = m_params.m_step * pToGScale;
  while (!splineIter.BeginAgain())
  {
    glsl::dvec2 pos = glsl::dvec2(splineIter.m_pos.x, splineIter.m_pos.y);
    glsl::dvec2 n = pToGScale * halfH * glsl::dvec2(-splineIter.m_dir.y, splineIter.m_dir.x);
    glsl::dvec2 d = pToGScale * halfW * glsl::dvec2(splineIter.m_dir.x, splineIter.m_dir.y);

    positions.push_back(glsl::Quad3(glsl::vec3(pos - d + n, m_params.m_depth),
                                    glsl::vec3(pos - d - n, m_params.m_depth),
                                    glsl::vec3(pos + d + n, m_params.m_depth),
                                    glsl::vec3(pos + d - n, m_params.m_depth)));
    splineIter.Step(step);
  }

  if (positions.empty())
    return;

  normals.resize(positions.size(), glsl::Quad2(0.0f, 0.0f, 0.0f, 0.0f,
                                               0.0f, 0.0f, 0.0f, 0.0f));

  m2::RectF const & rect = region.GetTexRect();
  float const textureNum = region.GetTextureNode().GetOffset();
  texCoord.resize(positions.size(), glsl::Quad3(glsl::vec3(rect.minX(), rect.maxY(), textureNum),
                                                glsl::vec3(rect.minX(), rect.minY(), textureNum),
                                                glsl::vec3(rect.maxX(), rect.maxY(), textureNum),
                                                glsl::vec3(rect.maxX(), rect.minY(), textureNum)));

  dp::GLState state(gpu::TEXTURING_PROGRAM, dp::GLState::DynamicGeometry);
  state.SetTextureSet(region.GetTextureNode().m_textureSet);
  state.SetBlending(dp::Blending(true));

  dp::AttributeProvider provider(3, positions.size() * VertexPerQuad);
  {
    dp::BindingInfo position(1/*, PathSymbolHandle::PositionAttributeID*/);
    dp::BindingDecl & decl = position.GetBindingDecl(0);
    decl.m_attributeName = "a_position";
    decl.m_componentCount = 3;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(0, position, dp::MakeStackRefPointer(positions.data()));
  }
  {
    dp::BindingInfo normal(1);
    dp::BindingDecl & decl = normal.GetBindingDecl(0);
    decl.m_attributeName = "a_normal";
    decl.m_componentCount = 2;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(1, normal, dp::MakeStackRefPointer(normals.data()));
  }
  {
    dp::BindingInfo texcoord(1);
    dp::BindingDecl & decl = texcoord.GetBindingDecl(0);
    decl.m_attributeName = "a_texCoords";
    decl.m_componentCount = 3;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(2, texcoord, dp::MakeStackRefPointer(texCoord.data()));
  }

  batcher->InsertListOfStrip(state, dp::MakeStackRefPointer(&provider), VertexPerQuad);
}

}
