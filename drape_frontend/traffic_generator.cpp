#include "drape_frontend/traffic_generator.hpp"

#include "drape_frontend/line_shape_helper.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shader_def.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/tile_utils.hpp"
#include "drape_frontend/traffic_renderer.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/glsl_func.hpp"
#include "drape/texture_manager.hpp"

#include "indexer/map_style_reader.hpp"

#include "base/logging.hpp"

#include "std/algorithm.hpp"

#include <functional>

using namespace std::placeholders;

namespace df
{
namespace
{
// Values of the following arrays are based on traffic-arrow texture.
static array<float, static_cast<size_t>(traffic::SpeedGroup::Count)> kCoordVOffsets =
{{
  0.75f,  // G0
  0.75f,  // G1
  0.75f,  // G2
  0.5f,   // G3
  0.25f,  // G4
  0.25f,  // G5
  0.75f,  // TempBlock
  0.0f,   // Unknown
}};

static array<float, static_cast<size_t>(traffic::SpeedGroup::Count)> kMinCoordU =
{{
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
  static unique_ptr<dp::BindingInfo> s_info;
  if (s_info == nullptr)
  {
    dp::BindingFiller<TrafficStaticVertex> filler(3);
    filler.FillDecl<TrafficStaticVertex::TPosition>("a_position");
    filler.FillDecl<TrafficStaticVertex::TNormal>("a_normal");
    filler.FillDecl<TrafficStaticVertex::TTexCoord>("a_colorTexCoord");
    s_info.reset(new dp::BindingInfo(filler.m_info));
  }
  return *s_info;
}

dp::BindingInfo const & GetTrafficLineStaticBindingInfo()
{
  static unique_ptr<dp::BindingInfo> s_info;
  if (s_info == nullptr)
  {
    dp::BindingFiller<TrafficLineStaticVertex> filler(2);
    filler.FillDecl<TrafficLineStaticVertex::TPosition>("a_position");
    filler.FillDecl<TrafficLineStaticVertex::TTexCoord>("a_colorTexCoord");
    s_info.reset(new dp::BindingInfo(filler.m_info));
  }
  return *s_info;
}

void SubmitStaticVertex(glsl::vec3 const & pivot, glsl::vec2 const & normal, float side,
                        float offsetFromStart, glsl::vec4 const & texCoord,
                        vector<TrafficStaticVertex> & staticGeom)
{
  staticGeom.emplace_back(pivot, TrafficStaticVertex::TNormal(normal, side, offsetFromStart), texCoord);
}

void GenerateCapTriangles(glsl::vec3 const & pivot, vector<glsl::vec2> const & normals,
                          dp::TextureManager::ColorRegion const & colorRegion,
                          vector<TrafficStaticVertex> & staticGeometry)
{
  float const kEps = 1e-5;
  glsl::vec4 const uv = glsl::vec4(glsl::ToVec2(colorRegion.GetTexRect().Center()), 0.0f, 0.0f);
  size_t const trianglesCount = normals.size() / 3;
  for (size_t j = 0; j < trianglesCount; j++)
  {
    SubmitStaticVertex(pivot, normals[3 * j],
                       glsl::length(normals[3 * j]) < kEps ? 0.0f : 1.0f, 0.0f, uv, staticGeometry);
    SubmitStaticVertex(pivot, normals[3 * j + 1],
                       glsl::length(normals[3 * j + 1]) < kEps ? 0.0f : 1.0f, 0.0f, uv, staticGeometry);
    SubmitStaticVertex(pivot, normals[3 * j + 2],
                       glsl::length(normals[3 * j + 2]) < kEps ? 0.0f : 1.0f, 0.0f, uv, staticGeometry);
  }
}

} // namespace

bool TrafficGenerator::m_simplifiedColorScheme = true;

void TrafficGenerator::Init()
{
  int constexpr kBatchersCount = 3;
  int constexpr kBatchSize = 65000;
  m_batchersPool = make_unique_dp<BatchersPool<TrafficBatcherKey, TrafficBatcherKeyComparator>>(
                                  kBatchersCount, bind(&TrafficGenerator::FlushGeometry, this, _1, _2, _3),
                                  kBatchSize, kBatchSize);

  m_providerLines.InitStream(0 /* stream index */, GetTrafficLineStaticBindingInfo(), nullptr);
  m_providerTriangles.InitStream(0 /* stream index */, GetTrafficStaticBindingInfo(), nullptr);
}

void TrafficGenerator::ClearGLDependentResources()
{
  InvalidateTexturesCache();
  m_batchersPool.reset();
}

void TrafficGenerator::FlushSegmentsGeometry(TileKey const & tileKey, TrafficSegmentsGeometry const & geom,
                                             ref_ptr<dp::TextureManager> textures)
{
  FillColorsCache(textures);
  ASSERT(m_colorsCacheValid, ());
  auto const texture = m_colorsCache[static_cast<size_t>(traffic::SpeedGroup::G0)].GetTexture();

  auto state = CreateGLState(gpu::TRAFFIC_PROGRAM, RenderState::GeometryLayer);
  state.SetColorTexture(texture);
  state.SetMaskTexture(textures->GetTrafficArrowTexture());

  auto lineState = CreateGLState(gpu::TRAFFIC_LINE_PROGRAM, RenderState::GeometryLayer);
  lineState.SetColorTexture(texture);
  lineState.SetDrawAsLine(true);

  static vector<RoadClass> const kRoadClasses = {RoadClass::Class0, RoadClass::Class1,
                                                 RoadClass::Class2};
  static float const kDepths[] = {2.0f, 1.0f, 0.0f};
  static vector<int> const kGenerateCapsZoomLevel = {14, 14, 16};

  for (auto geomIt = geom.begin(); geomIt != geom.end(); ++geomIt)
  {
    auto coloringIt = m_coloring.find(geomIt->first);
    if (coloringIt != m_coloring.end())
    {
      for (auto const & roadClass : kRoadClasses)
        m_batchersPool->ReserveBatcher(TrafficBatcherKey(geomIt->first, tileKey, roadClass));

      auto & coloring = coloringIt->second;
      for (size_t i = 0; i < geomIt->second.size(); i++)
      {
        traffic::TrafficInfo::RoadSegmentId const & sid = geomIt->second[i].first;
        auto segmentColoringIt = coloring.find(sid);
        if (segmentColoringIt != coloring.end())
        {
          // We do not generate geometry for unknown segments.
          if (segmentColoringIt->second == traffic::SpeedGroup::Unknown)
            continue;

          TrafficSegmentGeometry const & g = geomIt->second[i].second;
          ref_ptr<dp::Batcher> batcher =
              m_batchersPool->GetBatcher(TrafficBatcherKey(geomIt->first, tileKey, g.m_roadClass));

          float const depth = kDepths[static_cast<size_t>(g.m_roadClass)];

          ASSERT(m_colorsCacheValid, ());
          dp::TextureManager::ColorRegion const & colorRegion =
              m_colorsCache[static_cast<size_t>(segmentColoringIt->second)];
          float const vOffset = kCoordVOffsets[static_cast<size_t>(segmentColoringIt->second)];
          float const minU = kMinCoordU[static_cast<size_t>(segmentColoringIt->second)];

          int width = 0;
          if (TrafficRenderer::CanBeRendereredAsLine(g.m_roadClass, tileKey.m_zoomLevel, width))
          {
            vector<TrafficLineStaticVertex> staticGeometry;
            GenerateLineSegment(colorRegion, g.m_polyline, tileKey.GetGlobalRect().Center(), depth,
                                staticGeometry);
            if (staticGeometry.empty())
              continue;

            m_providerLines.Reset(static_cast<uint32_t>(staticGeometry.size()));
            m_providerLines.UpdateStream(0 /* stream index */, make_ref(staticGeometry.data()));

            dp::GLState curLineState = lineState;
            curLineState.SetLineWidth(width);
            batcher->InsertLineStrip(curLineState, make_ref(&m_providerLines));
          }
          else
          {
            vector<TrafficStaticVertex> staticGeometry;
            bool const generateCaps =
                (tileKey.m_zoomLevel > kGenerateCapsZoomLevel[static_cast<uint32_t>(g.m_roadClass)]);
            GenerateSegment(colorRegion, g.m_polyline, tileKey.GetGlobalRect().Center(),
                            generateCaps, depth, vOffset, minU, staticGeometry);
            if (staticGeometry.empty())
              continue;

            m_providerTriangles.Reset(static_cast<uint32_t>(staticGeometry.size()));
            m_providerTriangles.UpdateStream(0 /* stream index */, make_ref(staticGeometry.data()));
            batcher->InsertTriangleList(state, make_ref(&m_providerTriangles));
          }
        }
      }

      for (auto const & roadClass : kRoadClasses)
        m_batchersPool->ReleaseBatcher(TrafficBatcherKey(geomIt->first, tileKey, roadClass));
    }
  }

  GLFunctions::glFlush();
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

void TrafficGenerator::FlushGeometry(TrafficBatcherKey const & key, dp::GLState const & state,
                                     drape_ptr<dp::RenderBucket> && buffer)
{
  TrafficRenderData renderData(state);
  renderData.m_bucket = move(buffer);
  renderData.m_mwmId = key.m_mwmId;
  renderData.m_tileKey = key.m_tileKey;
  renderData.m_roadClass = key.m_roadClass;
  m_flushRenderDataFn(move(renderData));
}

void TrafficGenerator::GenerateSegment(dp::TextureManager::ColorRegion const & colorRegion,
                                       m2::PolylineD const & polyline, m2::PointD const & tileCenter,
                                       bool generateCaps, float depth, float vOffset, float minU,
                                       vector<TrafficStaticVertex> & staticGeometry)
{
  vector<m2::PointD> const & path = polyline.GetPoints();
  ASSERT_GREATER(path.size(), 1, ());

  size_t const kAverageSize = path.size() * 4;
  size_t const kAverageCapSize = 12;
  staticGeometry.reserve(staticGeometry.size() + kAverageSize + kAverageCapSize * 2);

  // Build geometry.
  glsl::vec2 firstPoint, firstTangent, firstLeftNormal, firstRightNormal;
  glsl::vec2 lastPoint, lastTangent, lastLeftNormal, lastRightNormal;
  bool firstFilled = false;

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

    // Fill first and last point, tangent and normals.
    if (!firstFilled)
    {
      firstPoint = p1;
      firstTangent = tangent;
      firstLeftNormal = leftNormal;
      firstRightNormal = rightNormal;
      firstFilled = true;
    }
    lastTangent = tangent;
    lastLeftNormal = leftNormal;
    lastRightNormal = rightNormal;
    lastPoint = p2;
    float const maskSize = static_cast<float>((path[i] - path[i - 1]).Length());

    glsl::vec3 const startPivot = glsl::vec3(p1, depth);
    glsl::vec3 const endPivot = glsl::vec3(p2, depth);
    SubmitStaticVertex(startPivot, rightNormal, -1.0f, 0.0f, uvStart, staticGeometry);
    SubmitStaticVertex(startPivot, leftNormal, 1.0f, 0.0f, uvStart, staticGeometry);
    SubmitStaticVertex(endPivot, rightNormal, -1.0f, maskSize, uvEnd, staticGeometry);
    SubmitStaticVertex(endPivot, rightNormal, -1.0f, maskSize, uvEnd, staticGeometry);
    SubmitStaticVertex(startPivot, leftNormal, 1.0f, 0.0f, uvStart, staticGeometry);
    SubmitStaticVertex(endPivot, leftNormal, 1.0f, maskSize, uvEnd, staticGeometry);
  }

  // Generate caps.
  if (generateCaps && firstFilled)
  {
    int const kSegmentsCount = 4;
    vector<glsl::vec2> normals;
    normals.reserve(kAverageCapSize);
    GenerateCapNormals(dp::RoundCap, firstLeftNormal, firstRightNormal, -firstTangent,
                       1.0f, true /* isStart */, normals, kSegmentsCount);
    GenerateCapTriangles(glsl::vec3(firstPoint, depth), normals, colorRegion, staticGeometry);

    normals.clear();
    GenerateCapNormals(dp::RoundCap, lastLeftNormal, lastRightNormal, lastTangent,
                       1.0f, false /* isStart */, normals, kSegmentsCount);
    GenerateCapTriangles(glsl::vec3(lastPoint, depth), normals, colorRegion, staticGeometry);
  }
}

void TrafficGenerator::GenerateLineSegment(dp::TextureManager::ColorRegion const & colorRegion,
                                           m2::PolylineD const & polyline, m2::PointD const & tileCenter,
                                           float depth, vector<TrafficLineStaticVertex> & staticGeometry)
{
  vector<m2::PointD> const & path = polyline.GetPoints();
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
  size_t constexpr kSpeedGroupsCount = static_cast<size_t>(traffic::SpeedGroup::Count);
  static array<df::ColorConstant, kSpeedGroupsCount> const kColorMap
  {{
    "TrafficG0",
    "TrafficG1",
    "TrafficG2",
    "TrafficG3",
    "TrafficG4",
    "TrafficG5",
    "TrafficTempBlock",
    "TrafficUnknown",
  }};

  static array<df::ColorConstant, kSpeedGroupsCount> const kColorMapRoute
  {{
    "RouteTrafficG0",
    "RouteTrafficG1",
    "RouteTrafficG2",
    "RouteTrafficG3",
    "TrafficG4",
    "TrafficG5",
    "TrafficTempBlock",
    "TrafficUnknown",
  }};

  size_t const index = static_cast<size_t>(CheckColorsSimplification(speedGroup));
  ASSERT_LESS(index, kSpeedGroupsCount, ());
  return route ? kColorMapRoute[index] : kColorMap[index];
}

void TrafficGenerator::FillColorsCache(ref_ptr<dp::TextureManager> textures)
{
  size_t constexpr kSpeedGroupsCount = static_cast<size_t>(traffic::SpeedGroup::Count);
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

} // namespace df
