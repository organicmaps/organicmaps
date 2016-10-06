#include "drape_frontend/traffic_generator.hpp"

#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/line_shape_helper.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/glsl_func.hpp"
#include "drape/shader_def.hpp"
#include "drape/texture_manager.hpp"

#include "indexer/map_style_reader.hpp"

#include "base/logging.hpp"

#include "std/algorithm.hpp"

namespace df
{

namespace
{

uint32_t const kDynamicStreamID = 0x7F;

dp::BindingInfo const & GetTrafficStaticBindingInfo()
{
  static unique_ptr<dp::BindingInfo> s_info;
  if (s_info == nullptr)
  {
    dp::BindingFiller<TrafficStaticVertex> filler(2);
    filler.FillDecl<TrafficStaticVertex::TPosition>("a_position");
    filler.FillDecl<TrafficStaticVertex::TNormal>("a_normal");
    s_info.reset(new dp::BindingInfo(filler.m_info));
  }
  return *s_info;
}

dp::BindingInfo const & GetTrafficDynamicBindingInfo()
{
  static unique_ptr<dp::BindingInfo> s_info;
  if (s_info == nullptr)
  {
    dp::BindingFiller<TrafficDynamicVertex> filler(1, kDynamicStreamID);
    filler.FillDecl<TrafficDynamicVertex::TTexCoord>("a_colorTexCoord");
    s_info.reset(new dp::BindingInfo(filler.m_info));
  }
  return *s_info;
}

void SubmitStaticVertex(glsl::vec3 const & pivot, glsl::vec2 const & normal, float side, float offsetFromStart,
                        vector<TrafficStaticVertex> & staticGeom)
{
  staticGeom.emplace_back(TrafficStaticVertex(pivot, TrafficStaticVertex::TNormal(normal, side, offsetFromStart)));
}

void SubmitDynamicVertex(glsl::vec2 const & texCoord, vector<TrafficDynamicVertex> & dynamicGeom)
{
  dynamicGeom.emplace_back(TrafficDynamicVertex(texCoord));
}

void GenerateCapTriangles(glsl::vec3 const & pivot, vector<glsl::vec2> const & normals,
                          dp::TextureManager::ColorRegion const & colorRegion,
                          vector<TrafficStaticVertex> & staticGeometry,
                          vector<TrafficDynamicVertex> & dynamicGeometry)
{
  float const kEps = 1e-5;
  glsl::vec2 const uv = glsl::ToVec2(colorRegion.GetTexRect().Center());
  size_t const trianglesCount = normals.size() / 3;
  for (int j = 0; j < trianglesCount; j++)
  {
    SubmitStaticVertex(pivot, normals[3 * j],
                       glsl::length(normals[3 * j]) < kEps ? 0.0f : 1.0f, 0.0f, staticGeometry);
    SubmitStaticVertex(pivot, normals[3 * j + 1],
                       glsl::length(normals[3 * j + 1]) < kEps ? 0.0f : 1.0f, 0.0f, staticGeometry);
    SubmitStaticVertex(pivot, normals[3 * j + 2],
                       glsl::length(normals[3 * j + 2]) < kEps ? 0.0f : 1.0f, 0.0f, staticGeometry);

    for (int k = 0; k < 3; k++)
      SubmitDynamicVertex(uv, dynamicGeometry);
  }
}

} // namespace

TrafficHandle::TrafficHandle(uint64_t segmentId, glsl::vec2 const & texCoord, size_t verticesCount)
  : OverlayHandle(FeatureID(), dp::Anchor::Center, 0, false)
  , m_segmentId(segmentId)
  , m_needUpdate(false)
{
  m_buffer.resize(verticesCount);
  for (size_t i = 0; i < m_buffer.size(); i++)
    m_buffer[i] = texCoord;
}

void TrafficHandle::GetAttributeMutation(ref_ptr<dp::AttributeBufferMutator> mutator) const
{
  if (!m_needUpdate)
    return;

  TOffsetNode const & node = GetOffsetNode(kDynamicStreamID);
  ASSERT(node.first.GetElementSize() == sizeof(TrafficDynamicVertex), ());
  ASSERT(node.second.m_count == m_buffer.size(), ());

  uint32_t const byteCount = m_buffer.size() * sizeof(TrafficDynamicVertex);
  void * buffer = mutator->AllocateMutationBuffer(byteCount);
  memcpy(buffer, m_buffer.data(), byteCount);

  dp::MutateNode mutateNode;
  mutateNode.m_region = node.second;
  mutateNode.m_data = make_ref(buffer);
  mutator->AddMutation(node.first, mutateNode);

  m_needUpdate = false;
}

bool TrafficHandle::Update(ScreenBase const & screen)
{
  UNUSED_VALUE(screen);
  return true;
}

bool TrafficHandle::IndexesRequired() const
{
  return false;
}

m2::RectD TrafficHandle::GetPixelRect(ScreenBase const & screen, bool perspective) const
{
  UNUSED_VALUE(screen);
  UNUSED_VALUE(perspective);
  return m2::RectD();
}

void TrafficHandle::GetPixelShape(ScreenBase const & screen, bool perspective, Rects & rects) const
{
  UNUSED_VALUE(screen);
  UNUSED_VALUE(perspective);
}

void TrafficHandle::SetTexCoord(glsl::vec2 const & texCoord)
{
  for (size_t i = 0; i < m_buffer.size(); i++)
    m_buffer[i] = texCoord;
  m_needUpdate = true;
}

uint64_t TrafficHandle::GetSegmentId() const
{
  return m_segmentId;
}

void TrafficGenerator::AddSegment(uint64_t segmentId, m2::PolylineD const & polyline)
{
  m_segments.insert(make_pair(segmentId, polyline));
}

void TrafficGenerator::ClearCache()
{
  m_colorsCache.clear();
  m_segmentsCache.clear();
}

vector<TrafficSegmentData> TrafficGenerator::GetSegmentsToUpdate(vector<TrafficSegmentData> const & trafficData) const
{
  vector<TrafficSegmentData> result;
  for (TrafficSegmentData const & segment : trafficData)
  {
    if (m_segmentsCache.find(segment.m_id) != m_segmentsCache.end())
      result.push_back(segment);
  }
  return result;
}

void TrafficGenerator::GetTrafficGeom(ref_ptr<dp::TextureManager> textures,
                                      vector<TrafficSegmentData> const & trafficData,
                                      vector<TrafficRenderData> & data)
{
  FillColorsCache(textures);

  dp::GLState state(gpu::TRAFFIC_PROGRAM, dp::GLState::GeometryLayer);
  state.SetColorTexture(m_colorsCache[TrafficSpeedBucket::Normal].GetTexture());
  state.SetMaskTexture(textures->GetTrafficArrowTexture());

  int const kZoomLevel = 10;
  uint32_t const kBatchSize = 5000;

  using TSegIter = TSegmentCollection::iterator;
  map<TileKey, list<pair<TSegIter, TrafficSpeedBucket>>> segmentsByTiles;
  for (TrafficSegmentData const & segment : trafficData)
  {
    // Check if a segment hasn't been added.
    auto it = m_segments.find(segment.m_id);
    if (it == m_segments.end())
      continue;

    // Check if a segment has already generated.
    if (m_segmentsCache.find(segment.m_id) != m_segmentsCache.end())
      continue;

    m_segmentsCache.insert(segment.m_id);
    TileKey const tileKey = GetTileKeyByPoint(it->second.GetLimitRect().Center(), kZoomLevel);
    segmentsByTiles[tileKey].emplace_back(make_pair(it, segment.m_speedBucket));
  }

  for (auto const & s : segmentsByTiles)
  {
    TileKey const & tileKey = s.first;
    dp::Batcher batcher(kBatchSize, kBatchSize);
    dp::SessionGuard guard(batcher, [&data, &tileKey](dp::GLState const & state, drape_ptr<dp::RenderBucket> && b)
    {
      TrafficRenderData bucket(state);
      bucket.m_bucket = move(b);
      bucket.m_tileKey = tileKey;
      data.emplace_back(move(bucket));
    });

    for (auto const & segmentPair : s.second)
    {
      TSegIter it = segmentPair.first;
      dp::TextureManager::ColorRegion const & colorRegion = m_colorsCache[segmentPair.second];
      m2::PolylineD const & polyline = it->second;

      vector<TrafficStaticVertex> staticGeometry;
      vector<TrafficDynamicVertex> dynamicGeometry;
      GenerateSegment(colorRegion, polyline, tileKey.GetGlobalRect().Center(), staticGeometry, dynamicGeometry);
      ASSERT_EQUAL(staticGeometry.size(), dynamicGeometry.size(), ());

      if ((staticGeometry.size() + dynamicGeometry.size()) == 0)
        continue;

      glsl::vec2 const uv = glsl::ToVec2(colorRegion.GetTexRect().Center());
      drape_ptr<dp::OverlayHandle> handle = make_unique_dp<TrafficHandle>(it->first, uv, staticGeometry.size());

      dp::AttributeProvider provider(2 /* stream count */, staticGeometry.size());
      provider.InitStream(0 /* stream index */, GetTrafficStaticBindingInfo(), make_ref(staticGeometry.data()));
      provider.InitStream(1 /* stream index */, GetTrafficDynamicBindingInfo(), make_ref(dynamicGeometry.data()));
      batcher.InsertTriangleList(state, make_ref(&provider), move(handle));
    }
  }

  GLFunctions::glFlush();
}

void TrafficGenerator::GenerateSegment(dp::TextureManager::ColorRegion const & colorRegion,
                                       m2::PolylineD const & polyline, m2::PointD const & tileCenter,
                                       vector<TrafficStaticVertex> & staticGeometry,
                                       vector<TrafficDynamicVertex> & dynamicGeometry)
{
  vector<m2::PointD> const & path = polyline.GetPoints();
  ASSERT_GREATER(path.size(), 1, ());

  size_t const kAverageSize = path.size() * 4;
  size_t const kAverageCapSize = 24;
  staticGeometry.reserve(staticGeometry.size() + kAverageSize + kAverageCapSize * 2);
  dynamicGeometry.reserve(dynamicGeometry.size() + kAverageSize + kAverageCapSize * 2);

  float const kDepth = 0.0f;

  // Build geometry.
  glsl::vec2 firstPoint, firstTangent, firstLeftNormal, firstRightNormal;
  glsl::vec2 lastPoint, lastTangent, lastLeftNormal, lastRightNormal;
  bool firstFilled = false;

  glsl::vec2 const uv = glsl::ToVec2(colorRegion.GetTexRect().Center());
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
    float const maskSize = (path[i] - path[i - 1]).Length();

    glsl::vec3 const startPivot = glsl::vec3(p1, kDepth);
    glsl::vec3 const endPivot = glsl::vec3(p2, kDepth);
    SubmitStaticVertex(startPivot, rightNormal, -1.0f, 0.0f, staticGeometry);
    SubmitStaticVertex(startPivot, leftNormal, 1.0f, 0.0f, staticGeometry);
    SubmitStaticVertex(endPivot, rightNormal, -1.0f, maskSize, staticGeometry);
    SubmitStaticVertex(endPivot, rightNormal, -1.0f, maskSize, staticGeometry);
    SubmitStaticVertex(startPivot, leftNormal, 1.0f, 0.0f, staticGeometry);
    SubmitStaticVertex(endPivot, leftNormal, 1.0f, maskSize, staticGeometry);
    for (int j = 0; j < 6; j++)
      SubmitDynamicVertex(uv, dynamicGeometry);
  }

  // Generate caps.
  if (firstFilled)
  {
    int const kSegmentsCount = 4;
    vector<glsl::vec2> normals;
    normals.reserve(kAverageCapSize);
    GenerateCapNormals(dp::RoundCap, firstLeftNormal, firstRightNormal, -firstTangent,
                       1.0f, true /* isStart */, normals, kSegmentsCount);
    GenerateCapTriangles(glsl::vec3(firstPoint, kDepth), normals, colorRegion,
                         staticGeometry, dynamicGeometry);

    normals.clear();
    GenerateCapNormals(dp::RoundCap, lastLeftNormal, lastRightNormal, lastTangent,
                       1.0f, false /* isStart */, normals, kSegmentsCount);
    GenerateCapTriangles(glsl::vec3(lastPoint, kDepth), normals, colorRegion,
                         staticGeometry, dynamicGeometry);
  }
}

void TrafficGenerator::FillColorsCache(ref_ptr<dp::TextureManager> textures)
{
  if (m_colorsCache.empty())
  {
    auto const & style = GetStyleReader().GetCurrentStyle();
    dp::TextureManager::ColorRegion colorRegion;
    textures->GetColorRegion(df::GetColorConstant(style, df::TrafficVerySlow), colorRegion);
    m_colorsCache[TrafficSpeedBucket::VerySlow] = colorRegion;

    textures->GetColorRegion(df::GetColorConstant(style, df::TrafficSlow), colorRegion);
    m_colorsCache[TrafficSpeedBucket::Slow] = colorRegion;

    textures->GetColorRegion(df::GetColorConstant(style, df::TrafficNormal), colorRegion);
    m_colorsCache[TrafficSpeedBucket::Normal] = colorRegion;

    m_colorsCacheRefreshed = true;
  }
}

unordered_map<int, glsl::vec2> TrafficGenerator::ProcessCacheRefreshing()
{
  unordered_map<int, glsl::vec2> result;
  for (auto it = m_colorsCache.begin(); it != m_colorsCache.end(); ++it)
    result[it->first] = glsl::ToVec2(it->second.GetTexRect().Center());
  m_colorsCacheRefreshed = false;
  return result;
}

} // namespace df

