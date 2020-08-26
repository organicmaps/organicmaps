#include "drape_frontend/area_shape.hpp"
#include "drape_frontend/render_state_extension.hpp"

#include "shaders/programs.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/texture_manager.hpp"
#include "drape/utils/vertex_decl.hpp"

#include "indexer/map_style_reader.hpp"

#include "base/buffer_vector.hpp"
#include "base/logging.hpp"

#include <algorithm>

namespace df
{
AreaShape::AreaShape(std::vector<m2::PointD> && triangleList, BuildingOutline && buildingOutline,
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

void AreaShape::DrawArea(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
                         m2::PointD const & colorUv, m2::PointD const & outlineUv,
                         ref_ptr<dp::Texture> texture) const
{
  glsl::vec2 const uv = glsl::ToVec2(colorUv);

  buffer_vector<gpu::AreaVertex, 128> vertexes;
  vertexes.resize(m_vertexes.size());
  std::transform(m_vertexes.begin(), m_vertexes.end(), vertexes.begin(), [&uv, this](m2::PointD const & vertex)
  {
    return gpu::AreaVertex(
      glsl::vec3(glsl::ToVec2(ConvertToLocal(m2::PointD(vertex), m_params.m_tileCenter, kShapeCoordScalar)),
                 m_params.m_depth), uv);
  });

  auto state = CreateRenderState(gpu::Program::Area, DepthLayer::GeometryLayer);
  state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
  state.SetColorTexture(texture);

  dp::AttributeProvider provider(1, static_cast<uint32_t>(vertexes.size()));
  provider.InitStream(0, gpu::AreaVertex::GetBindingInfo(), make_ref(vertexes.data()));
  batcher->InsertTriangleList(context, state, make_ref(&provider));

  // Generate outline.
  if (m_buildingOutline.m_generateOutline && !m_buildingOutline.m_indices.empty())
  {
    glsl::vec2 const ouv = glsl::ToVec2(outlineUv);

    std::vector<gpu::AreaVertex> vertices;
    vertices.reserve(m_buildingOutline.m_vertices.size());
    for (size_t i = 0; i < m_buildingOutline.m_vertices.size(); i++)
    {
      glsl::vec2 const pos = glsl::ToVec2(ConvertToLocal(m_buildingOutline.m_vertices[i],
                                                         m_params.m_tileCenter, kShapeCoordScalar));
      vertices.emplace_back(glsl::vec3(pos, m_params.m_depth), ouv);
    }

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

  double const maxU = bbox.SizeX() * m_params.m_baseGtoPScale / hatchingTexture->GetWidth();
  double const maxV = bbox.SizeY() * m_params.m_baseGtoPScale / hatchingTexture->GetHeight();

  buffer_vector<gpu::HatchingAreaVertex, 128> vertexes;
  vertexes.resize(m_vertexes.size());
  for (size_t i = 0; i < m_vertexes.size(); ++i)
  {
    vertexes[i].m_position = glsl::vec3(glsl::ToVec2(ConvertToLocal(m_vertexes[i], m_params.m_tileCenter,
                                                                    kShapeCoordScalar)), m_params.m_depth);
    vertexes[i].m_colorTexCoord = uv;
    vertexes[i].m_maskTexCoord.x = static_cast<float>(maxU * (m_vertexes[i].x - bbox.minX()) / bbox.SizeX());
    vertexes[i].m_maskTexCoord.y = static_cast<float>(maxV * (m_vertexes[i].y - bbox.minY()) / bbox.SizeY());
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
                           m2::PointD const & colorUv, m2::PointD const & outlineUv,
                           ref_ptr<dp::Texture> texture) const
{
  CHECK(!m_buildingOutline.m_indices.empty(), ());
  CHECK_EQUAL(m_buildingOutline.m_normals.size() * 2, m_buildingOutline.m_indices.size(), ());

  glsl::vec2 const uv = glsl::ToVec2(colorUv);

  std::vector<gpu::Area3dVertex> vertexes;
  vertexes.reserve(m_vertexes.size() + m_buildingOutline.m_normals.size() * 6);

  for (size_t i = 0; i < m_buildingOutline.m_normals.size(); i++)
  {
    int const startIndex = m_buildingOutline.m_indices[i * 2];
    int const endIndex = m_buildingOutline.m_indices[i * 2 + 1];

    glsl::vec2 const startPt = glsl::ToVec2(ConvertToLocal(m_buildingOutline.m_vertices[startIndex],
                                                           m_params.m_tileCenter, kShapeCoordScalar));
    glsl::vec2 const endPt = glsl::ToVec2(ConvertToLocal(m_buildingOutline.m_vertices[endIndex],
                                                         m_params.m_tileCenter, kShapeCoordScalar));

    glsl::vec3 normal(glsl::ToVec2(m_buildingOutline.m_normals[i]), 0.0f);
    vertexes.emplace_back(glsl::vec3(startPt, -m_params.m_minPosZ), normal, uv);
    vertexes.emplace_back(glsl::vec3(endPt, -m_params.m_minPosZ), normal, uv);
    vertexes.emplace_back(glsl::vec3(startPt, -m_params.m_posZ), normal, uv);

    vertexes.emplace_back(glsl::vec3(startPt, -m_params.m_posZ), normal, uv);
    vertexes.emplace_back(glsl::vec3(endPt, -m_params.m_minPosZ), normal, uv);
    vertexes.emplace_back(glsl::vec3(endPt, -m_params.m_posZ), normal, uv);
  }

  glsl::vec3 const normal(0.0f, 0.0f, -1.0f);
  for (auto const & vertex : m_vertexes)
  {
    glsl::vec2 const pt = glsl::ToVec2(ConvertToLocal(vertex, m_params.m_tileCenter, kShapeCoordScalar));
    vertexes.emplace_back(glsl::vec3(pt, -m_params.m_posZ), normal, uv);
  }

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

    std::vector<gpu::AreaVertex> vertices;
    vertices.reserve(m_buildingOutline.m_vertices.size());
    for (size_t i = 0; i < m_buildingOutline.m_vertices.size(); i++)
    {
      glsl::vec2 const pos = glsl::ToVec2(ConvertToLocal(m_buildingOutline.m_vertices[i],
                                                         m_params.m_tileCenter, kShapeCoordScalar));
      vertices.emplace_back(gpu::AreaVertex(glsl::vec3(pos, -m_params.m_posZ), ouv));
    }

    dp::AttributeProvider outlineProvider(1, static_cast<uint32_t>(vertices.size()));
    outlineProvider.InitStream(0, gpu::AreaVertex::GetBindingInfo(), make_ref(vertices.data()));
    batcher->InsertLineRaw(context, outlineState, make_ref(&outlineProvider), m_buildingOutline.m_indices);
  }
}
}  // namespace df
