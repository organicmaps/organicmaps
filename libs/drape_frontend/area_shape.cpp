#include "drape_frontend/area_shape.hpp"
#include "drape_frontend/render_state_extension.hpp"

#include "shaders/programs.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/hatching_decl.hpp"
#include "drape/texture_manager.hpp"
#include "drape/utils/vertex_decl.hpp"

#include "base/buffer_vector.hpp"

#include <cmath>

namespace df
{
// Analytic area patterns (hatches and solid-fill speckles) repeat every kHatchTilePx 'base' pixels - the
// size of the legacy mask tiles, kept so the on-screen scale is unchanged. The fragment shaders interpret
// v_maskTexCoords * kHatchTilePx as the in-tile pixel coordinate.
uint32_t constexpr kHatchTilePx = 16;
// The forest scatters bigger symbols on a coarser grid; keep in sync with kCellPx in area_forest.fsh.glsl.
uint32_t constexpr kForestTilePx = 32;
// The forest scatter hashes the integer cell index of v_maskTexCoords. CalcHatchingPhaseAnchor only keeps
// the fractional phase tile-stable, so that index would be offset per map-tile and large symbols would
// mismatch across seams. Anchoring to a COARSE grid of kPatternWrap tiles makes the index globally
// consistent (mod kPatternWrap) while keeping v_maskTexCoords small enough for float precision; the
// pattern then repeats every kPatternWrap tiles (~131072 px), which is never on screen.
uint32_t constexpr kPatternWrap = 4096;

namespace
{
// Maps an area-pattern key (hatch or solid-fill) to its analytic GPU program.
gpu::Program PatternProgram(std::string_view key)
{
  if (key == dp::k45dHatching)
    return gpu::Program::HatchingArea;
  if (key == dp::kDashHatching)
    return gpu::Program::HatchingAreaDash;
  if (key == dp::kStipplePattern)
    return gpu::Program::AreaStipple;
  if (key == dp::kSpecklePattern)
    return gpu::Program::AreaSpeckle;
  if (key == dp::kGridPattern)
    return gpu::Program::AreaGrid;
  if (key == dp::kForestPattern)
    return gpu::Program::AreaForest;
  if (key == dp::kForestConiferousPattern)
    return gpu::Program::AreaForestConiferous;
  if (key == dp::kForestDeciduousPattern)
    return gpu::Program::AreaForestDeciduous;
  CHECK(false, ("Unknown area pattern key:", key));
  return gpu::Program::Area;
}

// The forest family (mixed / coniferous / deciduous): same scatter, 32px tile, differing only in glyph set.
bool IsForestPattern(std::string_view key)
{
  return key == dp::kForestPattern || key == dp::kForestConiferousPattern || key == dp::kForestDeciduousPattern;
}

// Patterns whose fragment shader hashes the integer cell index for a jittered scatter; they need the coarse
// anchor so that index stays seam-consistent. The hatches and the regular grid use only the phase.
bool HashesCellIndex(std::string_view key)
{
  return key == dp::kStipplePattern || key == dp::kSpecklePattern || IsForestPattern(key);
}
}  // namespace

double CalcHatchingPhaseAnchor(double bboxMin, uint32_t maskSizePx, double baseGtoPScale)
{
  double const period = maskSizePx / baseGtoPScale;  // world units per mask repeat
  return std::floor(bboxMin / period) * period;
}

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

  if (m_params.m_depthLayer == DepthLayer::MwmBorderLayer)
    DrawMwmBorderArea(context, batcher, colorUv, region.GetTexture());
  else if (m_params.m_is3D)
    DrawArea3D(context, batcher, colorUv, outlineUv, region.GetTexture());
  else if (!m_params.m_areaPattern.empty())
    DrawPatternArea(context, batcher, colorUv, region.GetTexture(), m_params.m_areaPattern);
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

void AreaShape::DrawMwmBorderArea(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
                                  m2::PointD const & colorUv, ref_ptr<dp::Texture> texture) const
{
  glsl::vec2 const uv = glsl::ToVec2(colorUv);

  gpu::VBReservedSizeT<gpu::AreaVertex> vertexes;
  vertexes.reserve(m_vertexes.size());
  for (m2::PointD const & vertex : m_vertexes)
    vertexes.emplace_back(ToShapeVertex3(vertex), uv);

  auto state = CreateRenderState(gpu::Program::TransparentArea, DepthLayer::MwmBorderLayer);
  state.SetDepthTestEnabled(true);
  state.SetDepthFunction(dp::TestFunction::Less);
  state.SetColorTexture(texture);

  dp::AttributeProvider provider(1, static_cast<uint32_t>(vertexes.size()));
  provider.InitStream(0, gpu::AreaVertex::GetBindingInfo(), make_ref(vertexes.data()));
  batcher->InsertTriangleList(context, state, make_ref(&provider));
}

void AreaShape::DrawPatternArea(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
                                m2::PointD const & colorUv, ref_ptr<dp::Texture> texture,
                                std::string_view patternKey) const
{
  glsl::vec2 const uv = glsl::ToVec2(colorUv);

  m2::RectD bbox;
  for (auto const & v : m_vertexes)
    bbox.Add(v);

  // The forest scatters larger symbols on a coarser grid; the tile size must match the cell size in each
  // fragment shader. World units per tile repeat; the shader scales v_maskTexCoords back to in-tile px.
  uint32_t const tilePx = IsForestPattern(patternKey) ? kForestTilePx : kHatchTilePx;
  double const tilesPerWorld = m_params.m_baseGtoPScale / tilePx;

  // Anchor the repeated pattern to a global, period-aligned grid instead of the clipped bbox, so the
  // phase stays continuous across tile seams and LOD changes. See CalcHatchingPhaseAnchor / issue #12804.
  // Patterns that hash the integer cell index anchor to a coarse multiple (kPatternWrap tiles) to keep that
  // index seam-consistent across map tiles; phase-only patterns (hatches, regular grid) use a single tile.
  uint32_t const anchorPx = HashesCellIndex(patternKey) ? tilePx * kPatternWrap : tilePx;
  double const anchorX = CalcHatchingPhaseAnchor(bbox.minX(), anchorPx, m_params.m_baseGtoPScale);
  double const anchorY = CalcHatchingPhaseAnchor(bbox.minY(), anchorPx, m_params.m_baseGtoPScale);

  gpu::VBReservedSizeT<gpu::HatchingAreaVertex> vertexes;
  vertexes.reserve(m_vertexes.size());
  for (m2::PointD const & vertex : m_vertexes)
  {
    vertexes.emplace_back(ToShapeVertex3(vertex), uv,
                          glsl::vec2(static_cast<float>(tilesPerWorld * (vertex.x - anchorX)),
                                     static_cast<float>(tilesPerWorld * (vertex.y - anchorY))));
  }

  // All area patterns are computed analytically in the fragment shader (no mask texture, no mipmaps).
  auto state = CreateRenderState(PatternProgram(patternKey), DepthLayer::GeometryLayer);
  state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
  state.SetColorTexture(texture);

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
