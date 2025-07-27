#include "drape_frontend/traffic_generator.hpp"

#include "drape_frontend/line_shape_helper.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/tile_utils.hpp"
#include "drape_frontend/traffic_renderer.hpp"
#include "drape_frontend/visual_params.hpp"

#include "shaders/programs.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/glsl_func.hpp"
#include "drape/graphics_context.hpp"

#include "indexer/map_style_reader.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <array>
#include <memory>

using namespace std::placeholders;

namespace df
{
namespace
{
// Values of the following arrays are based on traffic-arrow texture.
static std::array<float, static_cast<size_t>(traffic::SpeedGroup::Count)> kCoordVOffsets = {{
    0.75f,  // G0
    0.75f,  // G1
    0.75f,  // G2
    0.5f,   // G3
    0.25f,  // G4
    0.25f,  // G5
    0.75f,  // TempBlock
    0.0f,   // Unknown
}};

static std::array<float, static_cast<size_t>(traffic::SpeedGroup::Count)> kMinCoordU = {{
    0.15f,  // G0
    0.15f,  // G1
    0.15f,  // G2
    0.33f,  // G3
    0.5f,   // G4
    0.0f,   // G5
    0.15f,  // TempBlock
    0.0f,   // Unknown
}};

dp::BindingInfo const & GetTrafficStaticBindingInfo()
{
  static std::unique_ptr<dp::BindingInfo> s_info;
  if (s_info == nullptr)
  {
    dp::BindingFiller<TrafficStaticVertex> filler(3);
    filler.FillDecl<TrafficStaticVertex::TPosition>("a_position");
    filler.FillDecl<TrafficStaticVertex::TNormal>("a_normal");
    filler.FillDecl<TrafficStaticVertex::TTexCoord>("a_colorTexCoord");
    s_info = std::make_unique<dp::BindingInfo>(filler.m_info);
  }
  return *s_info;
}

dp::BindingInfo const & GetTrafficLineStaticBindingInfo()
{
  static std::unique_ptr<dp::BindingInfo> s_info;
  if (s_info == nullptr)
  {
    dp::BindingFiller<TrafficLineStaticVertex> filler(2);
    filler.FillDecl<TrafficLineStaticVertex::TPosition>("a_position");
    filler.FillDecl<TrafficLineStaticVertex::TTexCoord>("a_colorTexCoord");
    s_info = std::make_unique<dp::BindingInfo>(filler.m_info);
  }
  return *s_info;
}

dp::BindingInfo const & GetTrafficCircleStaticBindingInfo()
{
  static std::unique_ptr<dp::BindingInfo> s_info;
  if (s_info == nullptr)
  {
    dp::BindingFiller<TrafficCircleStaticVertex> filler(3);
    filler.FillDecl<TrafficCircleStaticVertex::TPosition>("a_position");
    filler.FillDecl<TrafficCircleStaticVertex::TNormal>("a_normal");
    filler.FillDecl<TrafficCircleStaticVertex::TTexCoord>("a_colorTexCoord");
    s_info = std::make_unique<dp::BindingInfo>(filler.m_info);
  }
  return *s_info;
}

void SubmitStaticVertex(glsl::vec3 const & pivot, glsl::vec2 const & normal, float side, float offsetFromStart,
                        glsl::vec4 const & texCoord, std::vector<TrafficStaticVertex> & staticGeom)
{
  staticGeom.emplace_back(pivot, TrafficStaticVertex::TNormal(normal, side, offsetFromStart), texCoord);
}

void SubmitCircleStaticVertices(RoadClass roadClass, glsl::vec3 const & pivot, glsl::vec2 const & rightNormal,
                                glsl::vec2 const & uv, std::vector<TrafficCircleStaticVertex> & circlesGeometry)
{
  // Here we use an equilateral triangle to render circle (incircle of a triangle).
  static float const kSqrt3 = sqrt(3.0f);
  auto const p = glsl::vec4(pivot, static_cast<float>(roadClass));
  circlesGeometry.emplace_back(p, glsl::vec4(rightNormal, -kSqrt3, -1.0f), uv);
  circlesGeometry.emplace_back(p, glsl::vec4(rightNormal, kSqrt3, -1.0f), uv);
  circlesGeometry.emplace_back(p, glsl::vec4(rightNormal, 0.0f, 2.0f), uv);
}
}  // namespace

bool TrafficGenerator::m_simplifiedColorScheme = true;

void TrafficGenerator::Init()
{
  uint32_t constexpr kBatchersCount = 3;
  uint32_t constexpr kBatchSize = 5000;
  m_batchersPool = make_unique_dp<BatchersPool<TrafficBatcherKey, TrafficBatcherKeyComparator>>(
      kBatchersCount, std::bind(&TrafficGenerator::FlushGeometry, this, _1, _2, _3), kBatchSize, kBatchSize);

  uint32_t constexpr kCirclesBatchSize = 1000;
  m_circlesBatcher = make_unique_dp<dp::Batcher>(kCirclesBatchSize, kCirclesBatchSize);

  m_providerLines.InitStream(0 /* stream index */, GetTrafficLineStaticBindingInfo(), nullptr);
  m_providerTriangles.InitStream(0 /* stream index */, GetTrafficStaticBindingInfo(), nullptr);
  m_providerCircles.InitStream(0 /* stream index */, GetTrafficCircleStaticBindingInfo(), nullptr);
}

void TrafficGenerator::ClearContextDependentResources()
{
  InvalidateTexturesCache();
  m_batchersPool.reset();
  m_circlesBatcher.reset();
}

void TrafficGenerator::GenerateSegmentsGeometry(ref_ptr<dp::GraphicsContext> context, MwmSet::MwmId const & mwmId,
                                                TileKey const & tileKey, TrafficSegmentsGeometryValue const & geometry,
                                                traffic::TrafficInfo::Coloring const & coloring,
                                                ref_ptr<dp::TextureManager> texturesMgr)
{
  static std::array<int, 3> const kGenerateCirclesZoomLevel = {14, 14, 16};

  ASSERT(m_colorsCacheValid, ());
  auto const colorTexture = m_colorsCache[static_cast<size_t>(traffic::SpeedGroup::G0)].GetTexture();

  auto state = CreateRenderState(gpu::Program::Traffic, DepthLayer::GeometryLayer);
  state.SetColorTexture(colorTexture);
  state.SetMaskTexture(texturesMgr->GetTrafficArrowTexture());

  auto lineState = CreateRenderState(gpu::Program::TrafficLine, DepthLayer::GeometryLayer);
  lineState.SetColorTexture(colorTexture);
  lineState.SetDrawAsLine(true);

  auto circleState = CreateRenderState(gpu::Program::TrafficCircle, DepthLayer::GeometryLayer);
  circleState.SetColorTexture(colorTexture);

  bool isLeftHand = false;
  if (mwmId.GetInfo())
  {
    auto const & regionData = mwmId.GetInfo()->GetRegionData();
    isLeftHand = (regionData.Get(feature::RegionData::RD_DRIVING) == "l");
  }

  static std::array<float, 3> const kRoadClassDepths = {30.0f, 20.0f, 10.0f};

  for (auto const & geomPair : geometry)
  {
    auto const coloringIt = coloring.find(geomPair.first);
    if (coloringIt == coloring.cend() || coloringIt->second == traffic::SpeedGroup::Unknown)
      continue;

    auto const & colorRegion = m_colorsCache[static_cast<size_t>(coloringIt->second)];
    auto const vOffset = kCoordVOffsets[static_cast<size_t>(coloringIt->second)];
    auto const minU = kMinCoordU[static_cast<size_t>(coloringIt->second)];

    TrafficSegmentGeometry const & g = geomPair.second;
    ref_ptr<dp::Batcher> batcher = m_batchersPool->GetBatcher(TrafficBatcherKey(mwmId, tileKey, g.m_roadClass));
    batcher->SetBatcherHash(tileKey.GetHashValue(BatcherBucket::Traffic));

    auto const finalDepth =
        kRoadClassDepths[static_cast<size_t>(g.m_roadClass)] + static_cast<float>(coloringIt->second);

    int width = 0;
    if (TrafficRenderer::CanBeRenderedAsLine(g.m_roadClass, tileKey.m_zoomLevel, width))
    {
      std::vector<TrafficLineStaticVertex> staticGeometry;
      GenerateLineSegment(colorRegion, g.m_polyline, tileKey.GetGlobalRect().Center(), finalDepth, staticGeometry);
      if (staticGeometry.empty())
        continue;

      m_providerLines.Reset(static_cast<uint32_t>(staticGeometry.size()));
      m_providerLines.UpdateStream(0 /* stream index */, make_ref(staticGeometry.data()));

      dp::RenderState curLineState = lineState;
      curLineState.SetLineWidth(width);
      batcher->InsertLineStrip(context, curLineState, make_ref(&m_providerLines));
    }
    else
    {
      std::vector<TrafficStaticVertex> staticGeometry;
      bool const generateCircles =
          (tileKey.m_zoomLevel > kGenerateCirclesZoomLevel[static_cast<uint32_t>(g.m_roadClass)]);

      std::vector<TrafficCircleStaticVertex> circlesGeometry;
      GenerateSegment(g.m_roadClass, colorRegion, g.m_polyline, tileKey.GetGlobalRect().Center(), generateCircles,
                      finalDepth, vOffset, minU, isLeftHand, staticGeometry, circlesGeometry);
      if (staticGeometry.empty())
        continue;

      m_providerTriangles.Reset(static_cast<uint32_t>(staticGeometry.size()));
      m_providerTriangles.UpdateStream(0 /* stream index */, make_ref(staticGeometry.data()));
      batcher->InsertTriangleList(context, state, make_ref(&m_providerTriangles));

      if (circlesGeometry.empty())
        continue;

      m_providerCircles.Reset(static_cast<uint32_t>(circlesGeometry.size()));
      m_providerCircles.UpdateStream(0 /* stream index */, make_ref(circlesGeometry.data()));
      m_circlesBatcher->InsertTriangleList(context, circleState, make_ref(&m_providerCircles));
    }
  }
}

void TrafficGenerator::FlushSegmentsGeometry(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey,
                                             TrafficSegmentsGeometry const & geom, ref_ptr<dp::TextureManager> textures)
{
  FillColorsCache(textures);

  static std::array<RoadClass, 3> const kRoadClasses = {RoadClass::Class0, RoadClass::Class1, RoadClass::Class2};
  for (auto const & g : geom)
  {
    auto const & mwmId = g.first;
    auto coloringIt = m_coloring.find(mwmId);
    if (coloringIt == m_coloring.cend())
      continue;

    for (auto const & roadClass : kRoadClasses)
      m_batchersPool->ReserveBatcher(TrafficBatcherKey(mwmId, tileKey, roadClass));

    m_circlesBatcher->StartSession(
        [this, mwmId, tileKey](dp::RenderState const & state, drape_ptr<dp::RenderBucket> && renderBucket)
    { FlushGeometry(TrafficBatcherKey(mwmId, tileKey, RoadClass::Class0), state, std::move(renderBucket)); });

    GenerateSegmentsGeometry(context, mwmId, tileKey, g.second, coloringIt->second, textures);

    for (auto const & roadClass : kRoadClasses)
      m_batchersPool->ReleaseBatcher(context, TrafficBatcherKey(mwmId, tileKey, roadClass));

    m_circlesBatcher->EndSession(context);
  }

  context->Flush();
}

void TrafficGenerator::UpdateColoring(TrafficSegmentsColoring const & coloring)
{
  for (auto const & p : coloring)
    m_coloring[p.first] = p.second;
}

void TrafficGenerator::ClearCache()
{
  InvalidateTexturesCache();
  m_coloring.clear();
}

void TrafficGenerator::ClearCache(MwmSet::MwmId const & mwmId)
{
  m_coloring.erase(mwmId);
}

void TrafficGenerator::InvalidateTexturesCache()
{
  m_colorsCacheValid = false;
}

void TrafficGenerator::FlushGeometry(TrafficBatcherKey const & key, dp::RenderState const & state,
                                     drape_ptr<dp::RenderBucket> && buffer)
{
  TrafficRenderData renderData(state);
  renderData.m_bucket = std::move(buffer);
  renderData.m_mwmId = key.m_mwmId;
  renderData.m_tileKey = key.m_tileKey;
  renderData.m_roadClass = key.m_roadClass;
  m_flushRenderDataFn(std::move(renderData));
}

void TrafficGenerator::GenerateSegment(RoadClass roadClass, dp::TextureManager::ColorRegion const & colorRegion,
                                       m2::PolylineD const & polyline, m2::PointD const & tileCenter,
                                       bool generateCircles, float depth, float vOffset, float minU, bool isLeftHand,
                                       std::vector<TrafficStaticVertex> & staticGeometry,
                                       std::vector<TrafficCircleStaticVertex> & circlesGeometry)
{
  auto const & path = polyline.GetPoints();
  ASSERT_GREATER(path.size(), 1, ());

  size_t const kAverageSize = (path.size() - 1) * 6;
  staticGeometry.reserve(staticGeometry.size() + kAverageSize);
  circlesGeometry.reserve(circlesGeometry.size() + path.size() * 3);

  // Build geometry.
  glsl::vec2 lastPoint, lastRightNormal;
  bool firstFilled = false;
  auto const circleDepth = depth - 0.5f;

  glsl::vec4 const uvStart = glsl::vec4(glsl::ToVec2(colorRegion.GetTexRect().Center()), vOffset, 1.0f);
  glsl::vec4 const uvEnd = glsl::vec4(uvStart.x, uvStart.y, uvStart.z, minU);
  for (size_t i = 1; i < path.size(); ++i)
  {
    if (path[i].EqualDxDy(path[i - 1], 1.0E-5))
      continue;

    glsl::vec2 const p1 = glsl::ToVec2(MapShape::ConvertToLocal(path[i - 1], tileCenter, kShapeCoordScalar));
    glsl::vec2 const p2 = glsl::ToVec2(MapShape::ConvertToLocal(path[i], tileCenter, kShapeCoordScalar));
    glsl::vec2 tangent, leftNormal, rightNormal;
    CalculateTangentAndNormals(p1, p2, tangent, leftNormal, rightNormal);

    if (isLeftHand)
      std::swap(leftNormal, rightNormal);

    float const maskSize = static_cast<float>((path[i] - path[i - 1]).Length());

    glsl::vec3 const startPivot = glsl::vec3(p1, depth);
    glsl::vec3 const endPivot = glsl::vec3(p2, depth);
    if (isLeftHand)
    {
      SubmitStaticVertex(startPivot, leftNormal, 1.0f, 0.0f, uvStart, staticGeometry);
      SubmitStaticVertex(startPivot, rightNormal, -1.0f, 0.0f, uvStart, staticGeometry);
      SubmitStaticVertex(endPivot, leftNormal, 1.0f, maskSize, uvEnd, staticGeometry);
      SubmitStaticVertex(endPivot, leftNormal, 1.0f, maskSize, uvEnd, staticGeometry);
      SubmitStaticVertex(startPivot, rightNormal, -1.0f, 0.0f, uvStart, staticGeometry);
      SubmitStaticVertex(endPivot, rightNormal, -1.0f, maskSize, uvEnd, staticGeometry);
    }
    else
    {
      SubmitStaticVertex(startPivot, rightNormal, -1.0f, 0.0f, uvStart, staticGeometry);
      SubmitStaticVertex(startPivot, leftNormal, 1.0f, 0.0f, uvStart, staticGeometry);
      SubmitStaticVertex(endPivot, rightNormal, -1.0f, maskSize, uvEnd, staticGeometry);
      SubmitStaticVertex(endPivot, rightNormal, -1.0f, maskSize, uvEnd, staticGeometry);
      SubmitStaticVertex(startPivot, leftNormal, 1.0f, 0.0f, uvStart, staticGeometry);
      SubmitStaticVertex(endPivot, leftNormal, 1.0f, maskSize, uvEnd, staticGeometry);
    }

    if (generateCircles && !firstFilled)
    {
      SubmitCircleStaticVertices(roadClass, glsl::vec3(p1, circleDepth), rightNormal, glsl::vec2(uvStart),
                                 circlesGeometry);
    }

    firstFilled = true;
    lastRightNormal = rightNormal;
    lastPoint = p2;
  }

  if (generateCircles && firstFilled)
  {
    SubmitCircleStaticVertices(roadClass, glsl::vec3(lastPoint, circleDepth), lastRightNormal, glsl::vec2(uvStart),
                               circlesGeometry);
  }
}

void TrafficGenerator::GenerateLineSegment(dp::TextureManager::ColorRegion const & colorRegion,
                                           m2::PolylineD const & polyline, m2::PointD const & tileCenter, float depth,
                                           std::vector<TrafficLineStaticVertex> & staticGeometry)
{
  auto const & path = polyline.GetPoints();
  ASSERT_GREATER(path.size(), 1, ());

  size_t const kAverageSize = path.size();
  staticGeometry.reserve(staticGeometry.size() + kAverageSize);

  // Build geometry.
  glsl::vec2 const uv = glsl::ToVec2(colorRegion.GetTexRect().Center());
  for (size_t i = 0; i < path.size(); ++i)
  {
    glsl::vec2 const p = glsl::ToVec2(MapShape::ConvertToLocal(path[i], tileCenter, kShapeCoordScalar));
    staticGeometry.emplace_back(glsl::vec3(p, depth), uv);
  }
}

// static
void TrafficGenerator::SetSimplifiedColorSchemeEnabled(bool enabled)
{
  m_simplifiedColorScheme = enabled;
}

// static
traffic::SpeedGroup TrafficGenerator::CheckColorsSimplification(traffic::SpeedGroup speedGroup)
{
  // In simplified color scheme we reduce amount of speed groups visually.
  if (m_simplifiedColorScheme && speedGroup == traffic::SpeedGroup::G4)
    return traffic::SpeedGroup::G3;
  return speedGroup;
}

// static
df::ColorConstant TrafficGenerator::GetColorBySpeedGroup(traffic::SpeedGroup speedGroup, bool route)
{
  auto constexpr kSpeedGroupsCount = static_cast<size_t>(traffic::SpeedGroup::Count);
  static std::array<df::ColorConstant, kSpeedGroupsCount> const kColorMap{{
      "TrafficG0",
      "TrafficG1",
      "TrafficG2",
      "TrafficG3",
      "TrafficG4",
      "TrafficG5",
      "TrafficTempBlock",
      "TrafficUnknown",
  }};

  static std::array<df::ColorConstant, kSpeedGroupsCount> const kColorMapRoute{{
      "RouteTrafficG0",
      "RouteTrafficG1",
      "RouteTrafficG2",
      "RouteTrafficG3",
      "TrafficG4",
      "TrafficG5",
      "TrafficTempBlock",
      "TrafficUnknown",
  }};

  auto const index = static_cast<size_t>(CheckColorsSimplification(speedGroup));
  ASSERT_LESS(index, kSpeedGroupsCount, ());
  return route ? kColorMapRoute[index] : kColorMap[index];
}

void TrafficGenerator::FillColorsCache(ref_ptr<dp::TextureManager> textures)
{
  auto constexpr kSpeedGroupsCount = static_cast<size_t>(traffic::SpeedGroup::Count);
  if (!m_colorsCacheValid)
  {
    for (size_t i = 0; i < kSpeedGroupsCount; i++)
    {
      dp::TextureManager::ColorRegion colorRegion;
      auto const colorConstant = GetColorBySpeedGroup(static_cast<traffic::SpeedGroup>(i), false /* route */);
      textures->GetColorRegion(df::GetColorConstant(colorConstant), colorRegion);
      m_colorsCache[i] = colorRegion;
    }
    m_colorsCacheValid = true;
  }
}
}  // namespace df
