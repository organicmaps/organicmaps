#include "drape_frontend/area_shape.hpp"
#include "drape_frontend/render_state_extension.hpp"

#include "shaders/programs.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/texture_manager.hpp"
#include "drape/utils/vertex_decl.hpp"

#include "base/buffer_vector.hpp"

#include <algorithm>

namespace df
{

AreaShape::AreaShape(std::vector<m2::PointD> triangleList, BuildingOutline && buildingOutline,
                     AreaViewParams const & params)
  : m_vertexes(std::move(triangleList))
  , m_buildingOutline(std::move(buildingOutline))
  , m_params(params)
{}

void AreaShape::Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
                     ref_ptr<dp::TextureManager> textures) const
{
  dp::TextureManager::ColorRegion region;
  textures->GetColorRegion(m_params.m_color, region);
  m2::PointD const colorUv(region.GetTexRect().Center());
  m2::PointD outlineUv(0.0, 0.0);
  if (m_buildingOutline.m_generateOutline)
  {
    dp::TextureManager::ColorRegion outlineRegion;
    textures->GetColorRegion(m_params.m_outlineColor, outlineRegion);
    ASSERT_EQUAL(region.GetTexture(), outlineRegion.GetTexture(), ());
    outlineUv = outlineRegion.GetTexRect().Center();
  }

  if (m_params.m_is3D)
    DrawArea3D(context, batcher, colorUv, outlineUv, region.GetTexture());
  else if (m_params.m_hatching)
    DrawHatchingArea(context, batcher, colorUv, region.GetTexture(), textures->GetHatchingTexture());
  else
    DrawArea(context, batcher, colorUv, outlineUv, region.GetTexture());
}

void AreaShape::DrawArea(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher, m2::PointD const & colorUv,
                         m2::PointD const & outlineUv, ref_ptr<dp::Texture> texture) const
{
  glsl::vec2 const uv = glsl::ToVec2(colorUv);

  gpu::VBReservedSizeT<gpu::AreaVertex> vertexes;
  vertexes.reserve(m_vertexes.size());
  for (m2::PointD const & vertex : m_vertexes)
    vertexes.emplace_back(ToShapeVertex3(vertex), uv);

  auto const areaProgram = m_params.m_color.GetAlpha() == 255 ? gpu::Program::Area : gpu::Program::TransparentArea;
  auto state = CreateRenderState(areaProgram, DepthLayer::GeometryLayer);
  state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
  state.SetColorTexture(texture);

  dp::AttributeProvider provider(1, static_cast<uint32_t>(vertexes.size()));
  provider.InitStream(0, gpu::AreaVertex::GetBindingInfo(), make_ref(vertexes.data()));
  batcher->InsertTriangleList(context, state, make_ref(&provider));

  // Generate outline.
  if (m_buildingOutline.m_generateOutline && !m_buildingOutline.m_indices.empty())
  {
    glsl::vec2 const ouv = glsl::ToVec2(outlineUv);

    gpu::VBReservedSizeT<gpu::AreaVertex> vertices;
    vertices.reserve(m_buildingOutline.m_vertices.size());
    for (m2::PointD const & vertex : m_buildingOutline.m_vertices)
      vertices.emplace_back(ToShapeVertex3(vertex), ouv);

    auto outlineState = CreateRenderState(gpu::Program::AreaOutline, DepthLayer::GeometryLayer);
    outlineState.SetDepthTestEnabled(m_params.m_depthTestEnabled);
    outlineState.SetColorTexture(texture);
    outlineState.SetDrawAsLine(true);

    dp::AttributeProvider outlineProvider(1, static_cast<uint32_t>(vertices.size()));
    outlineProvider.InitStream(0, gpu::AreaVertex::GetBindingInfo(), make_ref(vertices.data()));
    batcher->InsertLineRaw(context, outlineState, make_ref(&outlineProvider), m_buildingOutline.m_indices);
  }
}

void AreaShape::DrawHatchingArea(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
                                 m2::PointD const & colorUv, ref_ptr<dp::Texture> texture,
                                 ref_ptr<dp::Texture> hatchingTexture) const
{
  glsl::vec2 const uv = glsl::ToVec2(colorUv);

  m2::RectD bbox;
  for (auto const & v : m_vertexes)
    bbox.Add(v);

  double const maxU = m_params.m_baseGtoPScale / hatchingTexture->GetWidth();
  double const maxV = m_params.m_baseGtoPScale / hatchingTexture->GetHeight();

  gpu::VBReservedSizeT<gpu::HatchingAreaVertex> vertexes;
  vertexes.reserve(m_vertexes.size());
  for (m2::PointD const & vertex : m_vertexes)
  {
    vertexes.emplace_back(ToShapeVertex3(vertex), uv,
                          glsl::vec2(static_cast<float>(maxU * (vertex.x - bbox.minX())),
                                     static_cast<float>(maxV * (vertex.y - bbox.minY()))));
  }

  auto state = CreateRenderState(gpu::Program::HatchingArea, DepthLayer::GeometryLayer);
  state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
  state.SetColorTexture(texture);
  state.SetMaskTexture(hatchingTexture);
  state.SetTextureFilter(dp::TextureFilter::Linear);

  dp::AttributeProvider provider(1, static_cast<uint32_t>(vertexes.size()));
  provider.InitStream(0, gpu::HatchingAreaVertex::GetBindingInfo(), make_ref(vertexes.data()));
  batcher->InsertTriangleList(context, state, make_ref(&provider));
}

void AreaShape::DrawArea3D(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
                           m2::PointD const & colorUv, m2::PointD const & outlineUv, ref_ptr<dp::Texture> texture) const
{
  CHECK(!m_buildingOutline.m_indices.empty(), ());
  CHECK_EQUAL(m_buildingOutline.m_normals.size() * 2, m_buildingOutline.m_indices.size(), ());

  glsl::vec2 const uv = glsl::ToVec2(colorUv);

  gpu::VBReservedSizeT<gpu::Area3dVertex> vertexes;
  vertexes.reserve(m_vertexes.size() + m_buildingOutline.m_normals.size() * 6);

  for (size_t i = 0; i < m_buildingOutline.m_normals.size(); i++)
  {
    int const startIndex = m_buildingOutline.m_indices[i * 2];
    int const endIndex = m_buildingOutline.m_indices[i * 2 + 1];

    glsl::vec2 const startPt = ToShapeVertex2(m_buildingOutline.m_vertices[startIndex]);
    glsl::vec2 const endPt = ToShapeVertex2(m_buildingOutline.m_vertices[endIndex]);

    glsl::vec3 normal(glsl::ToVec2(m_buildingOutline.m_normals[i]), 0.0f);
    vertexes.emplace_back(glsl::vec3(startPt, -m_params.m_minPosZ), normal, uv);
    vertexes.emplace_back(glsl::vec3(endPt, -m_params.m_minPosZ), normal, uv);
    vertexes.emplace_back(glsl::vec3(startPt, -m_params.m_posZ), normal, uv);

    vertexes.emplace_back(glsl::vec3(startPt, -m_params.m_posZ), normal, uv);
    vertexes.emplace_back(glsl::vec3(endPt, -m_params.m_minPosZ), normal, uv);
    vertexes.emplace_back(glsl::vec3(endPt, -m_params.m_posZ), normal, uv);
  }

  glsl::vec3 const normal(0.0f, 0.0f, -1.0f);
  for (m2::PointD const & vertex : m_vertexes)
    vertexes.emplace_back(glsl::vec3(ToShapeVertex2(vertex), -m_params.m_posZ), normal, uv);

  auto state = CreateRenderState(gpu::Program::Area3d, DepthLayer::Geometry3dLayer);
  state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
  state.SetColorTexture(texture);
  state.SetBlending(dp::Blending(false /* isEnabled */));

  dp::AttributeProvider provider(1, static_cast<uint32_t>(vertexes.size()));
  provider.InitStream(0, gpu::Area3dVertex::GetBindingInfo(), make_ref(vertexes.data()));
  batcher->InsertTriangleList(context, state, make_ref(&provider));

  // Generate outline.
  if (m_buildingOutline.m_generateOutline)
  {
    glsl::vec2 const ouv = glsl::ToVec2(outlineUv);

    auto outlineState = CreateRenderState(gpu::Program::Area3dOutline, DepthLayer::Geometry3dLayer);
    outlineState.SetDepthTestEnabled(m_params.m_depthTestEnabled);
    outlineState.SetColorTexture(texture);
    outlineState.SetBlending(dp::Blending(false /* isEnabled */));
    outlineState.SetDrawAsLine(true);

    gpu::VBReservedSizeT<gpu::AreaVertex> olVertexes;
    olVertexes.reserve(m_buildingOutline.m_vertices.size());
    for (m2::PointD const & vertex : m_buildingOutline.m_vertices)
      olVertexes.emplace_back(glsl::vec3(ToShapeVertex2(vertex), -m_params.m_posZ), ouv);

    dp::AttributeProvider outlineProvider(1, static_cast<uint32_t>(olVertexes.size()));
    outlineProvider.InitStream(0, gpu::AreaVertex::GetBindingInfo(), make_ref(olVertexes.data()));
    batcher->InsertLineRaw(context, outlineState, make_ref(&outlineProvider), m_buildingOutline.m_indices);
  }
}
}  // namespace df
