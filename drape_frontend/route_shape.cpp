#include "drape_frontend/route_shape.hpp"
#include "drape_frontend/line_shape_helper.hpp"
#include "drape_frontend/shader_def.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/tile_utils.hpp"
#include "drape_frontend/traffic_generator.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/glsl_func.hpp"
#include "drape/glsl_types.hpp"
#include "drape/texture_manager.hpp"

#include "base/logging.hpp"

namespace df
{
namespace
{
float const kLeftSide = 1.0f;
float const kCenter = 0.0f;
float const kRightSide = -1.0f;

float const kRouteDepth = 100.0f;
float const kArrowsDepth = 200.0f;
float const kDepthPerSubroute = 200.0f;

void GetArrowTextureRegion(ref_ptr<dp::TextureManager> textures,
                           dp::TextureManager::SymbolRegion & region)
{
  textures->GetSymbolRegion("route-arrow", region);
}

std::vector<m2::PointD> CalculatePoints(m2::PolylineD const & polyline,
                                        double start, double end)
{
  std::vector<m2::PointD> result;
  result.reserve(polyline.GetSize() / 4);

  auto addIfNotExist = [&result](m2::PointD const & pnt)
  {
    if (result.empty() || result.back() != pnt)
      result.push_back(pnt);
  };

  std::vector<m2::PointD> const & path = polyline.GetPoints();
  double len = 0;
  bool started = false;
  for (size_t i = 0; i + 1 < path.size(); i++)
  {
    double const dist = (path[i + 1] - path[i]).Length();
    if (fabs(dist) < 1e-5)
      continue;

    double const l = len + dist;
    if (!started && start >= len && start <= l)
    {
      double const k = (start - len) / dist;
      addIfNotExist(path[i] + (path[i + 1] - path[i]) * k);
      started = true;
    }
    if (!started)
    {
      len = l;
      continue;
    }

    if (end >= len && end <= l)
    {
      double const k = (end - len) / dist;
      addIfNotExist(path[i] + (path[i + 1] - path[i]) * k);
      break;
    }
    else
    {
      addIfNotExist(path[i + 1]);
    }
    len = l;
  }
  return result;
}

float SideByNormal(glsl::vec2 const & normal, bool isLeft)
{
  float const kEps = 1e-5;
  float const side = isLeft ? kLeftSide : kRightSide;
  return glsl::length(normal) < kEps ? kCenter : side;
}

void GenerateJoinsTriangles(glsl::vec3 const & pivot, std::vector<glsl::vec2> const & normals,
                            glsl::vec4 const & color, glsl::vec2 const & length, bool isLeft,
                            RouteShape::TGeometryBuffer & joinsGeometry)
{
  size_t const trianglesCount = normals.size() / 3;
  for (size_t j = 0; j < trianglesCount; j++)
  {
    glsl::vec3 const len1 = glsl::vec3(length.x, length.y, SideByNormal(normals[3 * j], isLeft));
    glsl::vec3 const len2 = glsl::vec3(length.x, length.y, SideByNormal(normals[3 * j + 1], isLeft));
    glsl::vec3 const len3 = glsl::vec3(length.x, length.y, SideByNormal(normals[3 * j + 2], isLeft));

    joinsGeometry.push_back(RouteShape::RV(pivot, normals[3 * j], len1, color));
    joinsGeometry.push_back(RouteShape::RV(pivot, normals[3 * j + 1], len2, color));
    joinsGeometry.push_back(RouteShape::RV(pivot, normals[3 * j + 2], len3, color));
  }
}

glsl::vec2 GetUV(m2::RectF const & texRect, float normU, float normV)
{
  return glsl::vec2(texRect.minX() * (1.0f - normU) + texRect.maxX() * normU,
                    texRect.minY() * (1.0f - normV) + texRect.maxY() * normV);
}

glsl::vec2 GetUV(m2::RectF const & texRect, glsl::vec2 const & uv)
{
  return GetUV(texRect, uv.x, uv.y);
}

void GenerateArrowsTriangles(glsl::vec4 const & pivot, std::vector<glsl::vec2> const & normals,
                             m2::RectF const & texRect, std::vector<glsl::vec2> const & uv,
                             bool normalizedUV, RouteShape::TArrowGeometryBuffer & joinsGeometry)
{
  size_t const trianglesCount = normals.size() / 3;
  for (size_t j = 0; j < trianglesCount; j++)
  {
    joinsGeometry.push_back(RouteShape::AV(pivot, normals[3 * j],
                            normalizedUV ? GetUV(texRect, uv[3 * j]) : uv[3 * j]));
    joinsGeometry.push_back(RouteShape::AV(pivot, normals[3 * j + 1],
                            normalizedUV ? GetUV(texRect, uv[3 * j + 1]) : uv[3 * j + 1]));
    joinsGeometry.push_back(RouteShape::AV(pivot, normals[3 * j + 2],
                            normalizedUV ? GetUV(texRect, uv[3 * j + 2]) : uv[3 * j + 2]));
  }
}

} // namespace

void RouteShape::PrepareGeometry(std::vector<m2::PointD> const & path, m2::PointD const & pivot,
                                 std::vector<glsl::vec4> const & segmentsColors, float baseDepth,
                                 TGeometryBuffer & geometry, TGeometryBuffer & joinsGeometry)
{
  ASSERT(path.size() > 1, ());

  // Construct segments.
  std::vector<LineSegment> segments;
  segments.reserve(path.size() - 1);
  ConstructLineSegments(path, segmentsColors, segments);

  if (segments.empty())
    return;

  // Build geometry.
  float length = 0.0f;
  for (size_t i = 0; i < segments.size() ; ++i)
    length += glsl::length(segments[i].m_points[EndPoint] - segments[i].m_points[StartPoint]);

  float depth = baseDepth;
  float const depthStep = kRouteDepth / (1 + segments.size());
  for (int i = static_cast<int>(segments.size() - 1); i >= 0; i--)
  {
    UpdateNormals(&segments[i], (i > 0) ? &segments[i - 1] : nullptr,
                 (i < static_cast<int>(segments.size()) - 1) ? &segments[i + 1] : nullptr);

    // Generate main geometry.
    m2::PointD const startPt = MapShape::ConvertToLocal(glsl::FromVec2(segments[i].m_points[StartPoint]),
                                                        pivot, kShapeCoordScalar);
    m2::PointD const endPt = MapShape::ConvertToLocal(glsl::FromVec2(segments[i].m_points[EndPoint]),
                                                      pivot, kShapeCoordScalar);

    glsl::vec3 const startPivot = glsl::vec3(glsl::ToVec2(startPt), depth);
    glsl::vec3 const endPivot = glsl::vec3(glsl::ToVec2(endPt), depth);
    depth += depthStep;

    float const startLength = length - glsl::length(segments[i].m_points[EndPoint] - segments[i].m_points[StartPoint]);

    glsl::vec2 const leftNormalStart = GetNormal(segments[i], true /* isLeft */, StartNormal);
    glsl::vec2 const rightNormalStart = GetNormal(segments[i], false /* isLeft */, StartNormal);
    glsl::vec2 const leftNormalEnd = GetNormal(segments[i], true /* isLeft */, EndNormal);
    glsl::vec2 const rightNormalEnd = GetNormal(segments[i], false /* isLeft */, EndNormal);

    float const projLeftStart = -segments[i].m_leftWidthScalar[StartPoint].y;
    float const projLeftEnd = segments[i].m_leftWidthScalar[EndPoint].y;
    float const projRightStart = -segments[i].m_rightWidthScalar[StartPoint].y;
    float const projRightEnd = segments[i].m_rightWidthScalar[EndPoint].y;

    geometry.push_back(RV(startPivot, glsl::vec2(0, 0),
                          glsl::vec3(startLength, 0, kCenter), segments[i].m_color));
    geometry.push_back(RV(startPivot, leftNormalStart,
                          glsl::vec3(startLength, projLeftStart, kLeftSide), segments[i].m_color));
    geometry.push_back(RV(endPivot, glsl::vec2(0, 0),
                          glsl::vec3(length, 0, kCenter), segments[i].m_color));
    geometry.push_back(RV(endPivot, leftNormalEnd,
                          glsl::vec3(length, projLeftEnd, kLeftSide), segments[i].m_color));

    geometry.push_back(RV(startPivot, rightNormalStart,
                          glsl::vec3(startLength, projRightStart, kRightSide), segments[i].m_color));
    geometry.push_back(RV(startPivot, glsl::vec2(0, 0),
                          glsl::vec3(startLength, 0, kCenter), segments[i].m_color));
    geometry.push_back(RV(endPivot, rightNormalEnd,
                          glsl::vec3(length, projRightEnd, kRightSide), segments[i].m_color));
    geometry.push_back(RV(endPivot, glsl::vec2(0, 0),
                          glsl::vec3(length, 0, kCenter), segments[i].m_color));

    // Generate joins.
    if (segments[i].m_generateJoin && i < static_cast<int>(segments.size()) - 1)
    {
      glsl::vec2 n1 = segments[i].m_hasLeftJoin[EndPoint] ? segments[i].m_leftNormals[EndPoint] :
                                                            segments[i].m_rightNormals[EndPoint];
      glsl::vec2 n2 = segments[i + 1].m_hasLeftJoin[StartPoint] ? segments[i + 1].m_leftNormals[StartPoint] :
                                                                  segments[i + 1].m_rightNormals[StartPoint];

      float widthScalar = segments[i].m_hasLeftJoin[EndPoint] ? segments[i].m_rightWidthScalar[EndPoint].x :
                                                                segments[i].m_leftWidthScalar[EndPoint].x;

      std::vector<glsl::vec2> normals;
      normals.reserve(24);
      GenerateJoinNormals(dp::RoundJoin, n1, n2, 1.0f, segments[i].m_hasLeftJoin[EndPoint],
                          widthScalar, normals);

      GenerateJoinsTriangles(endPivot, normals, segments[i].m_color, glsl::vec2(length, 0),
                             segments[i].m_hasLeftJoin[EndPoint], joinsGeometry);
    }

    // Generate caps.
    if (i == 0)
    {
      std::vector<glsl::vec2> normals;
      normals.reserve(24);
      GenerateCapNormals(dp::RoundCap, segments[i].m_leftNormals[StartPoint],
                         segments[i].m_rightNormals[StartPoint], -segments[i].m_tangent,
                         1.0f, true /* isStart */, normals);

      GenerateJoinsTriangles(startPivot, normals, segments[i].m_color, glsl::vec2(startLength, 0),
                             true, joinsGeometry);
    }

    if (i == static_cast<int>(segments.size()) - 1)
    {
      std::vector<glsl::vec2> normals;
      normals.reserve(24);
      GenerateCapNormals(dp::RoundCap, segments[i].m_leftNormals[EndPoint],
                         segments[i].m_rightNormals[EndPoint], segments[i].m_tangent,
                         1.0f, false /* isStart */, normals);

      GenerateJoinsTriangles(endPivot, normals, segments[i].m_color, glsl::vec2(length, 0),
                             true, joinsGeometry);
    }

    length = startLength;
  }
}

void RouteShape::PrepareArrowGeometry(std::vector<m2::PointD> const & path, m2::PointD const & pivot,
                                      m2::RectF const & texRect, float depthStep, float depth,
                                      TArrowGeometryBuffer & geometry, TArrowGeometryBuffer & joinsGeometry)
{
  ASSERT(path.size() > 1, ());

  // Construct segments.
  std::vector<LineSegment> segments;
  segments.reserve(path.size() - 1);
  ConstructLineSegments(path, std::vector<glsl::vec4>(), segments);

  m2::RectF tr = texRect;
  tr.setMinX(static_cast<float>(texRect.minX() * (1.0 - kArrowTailSize) + texRect.maxX() * kArrowTailSize));
  tr.setMaxX(static_cast<float>(texRect.minX() * kArrowHeadSize + texRect.maxX() * (1.0 - kArrowHeadSize)));

  // Build geometry.
  float const depthInc = depthStep / (segments.size() + 1);
  for (size_t i = 0; i < segments.size(); i++)
  {
    UpdateNormals(&segments[i], (i > 0) ? &segments[i - 1] : nullptr,
                 (i < segments.size() - 1) ? &segments[i + 1] : nullptr);

    // Generate main geometry.
    m2::PointD const startPt = MapShape::ConvertToLocal(glsl::FromVec2(segments[i].m_points[StartPoint]),
                                                        pivot, kShapeCoordScalar);
    m2::PointD const endPt = MapShape::ConvertToLocal(glsl::FromVec2(segments[i].m_points[EndPoint]),
                                                      pivot, kShapeCoordScalar);

    glsl::vec4 const startPivot = glsl::vec4(glsl::ToVec2(startPt), depth, 1.0);
    glsl::vec4 const endPivot = glsl::vec4(glsl::ToVec2(endPt), depth, 1.0);
    depth += depthInc;

    glsl::vec2 const leftNormalStart = GetNormal(segments[i], true /* isLeft */, StartNormal);
    glsl::vec2 const rightNormalStart = GetNormal(segments[i], false /* isLeft */, StartNormal);
    glsl::vec2 const leftNormalEnd = GetNormal(segments[i], true /* isLeft */, EndNormal);
    glsl::vec2 const rightNormalEnd = GetNormal(segments[i], false /* isLeft */, EndNormal);

    glsl::vec2 const uvCenter = GetUV(tr, 0.5f, 0.5f);
    glsl::vec2 const uvLeft = GetUV(tr, 0.5f, 0.0f);
    glsl::vec2 const uvRight = GetUV(tr, 0.5f, 1.0f);

    geometry.push_back(AV(startPivot, glsl::vec2(0, 0), uvCenter));
    geometry.push_back(AV(startPivot, leftNormalStart, uvLeft));
    geometry.push_back(AV(endPivot, glsl::vec2(0, 0), uvCenter));
    geometry.push_back(AV(endPivot, leftNormalEnd, uvLeft));

    geometry.push_back(AV(startPivot, rightNormalStart, uvRight));
    geometry.push_back(AV(startPivot, glsl::vec2(0, 0), uvCenter));
    geometry.push_back(AV(endPivot, rightNormalEnd, uvRight));
    geometry.push_back(AV(endPivot, glsl::vec2(0, 0), uvCenter));

    // Generate joins.
    if (segments[i].m_generateJoin && i < segments.size() - 1)
    {
      glsl::vec2 n1 = segments[i].m_hasLeftJoin[EndPoint] ? segments[i].m_leftNormals[EndPoint] :
                                                            segments[i].m_rightNormals[EndPoint];
      glsl::vec2 n2 = segments[i + 1].m_hasLeftJoin[StartPoint] ? segments[i + 1].m_leftNormals[StartPoint] :
                                                                  segments[i + 1].m_rightNormals[StartPoint];

      float widthScalar = segments[i].m_hasLeftJoin[EndPoint] ? segments[i].m_rightWidthScalar[EndPoint].x :
                                                                segments[i].m_leftWidthScalar[EndPoint].x;

      int const kAverageSize = 24;
      std::vector<glsl::vec2> normals;
      normals.reserve(kAverageSize);
      std::vector<glsl::vec2> uv;
      uv.reserve(kAverageSize);

      GenerateJoinNormals(dp::RoundJoin, n1, n2, 1.0f, segments[i].m_hasLeftJoin[EndPoint],
                          widthScalar, normals, &uv);

      ASSERT_EQUAL(normals.size(), uv.size(), ());

      GenerateArrowsTriangles(endPivot, normals, tr, uv, true /* normalizedUV */, joinsGeometry);
    }

    // Generate arrow head.
    if (i == segments.size() - 1)
    {
      std::vector<glsl::vec2> normals =
      {
        segments[i].m_rightNormals[EndPoint],
        segments[i].m_leftNormals[EndPoint],
        kArrowHeadFactor * segments[i].m_tangent
      };
      float const u = 1.0f - static_cast<float>(kArrowHeadSize);
      std::vector<glsl::vec2> uv = { glsl::vec2(u, 1.0f), glsl::vec2(u, 0.0f), glsl::vec2(1.0f, 0.5f) };
      glsl::vec4 const headPivot = glsl::vec4(glsl::ToVec2(endPt), depth, 1.0);
      depth += depthInc;
      GenerateArrowsTriangles(headPivot, normals, texRect, uv, true /* normalizedUV */, joinsGeometry);
    }

    // Generate arrow tail.
    if (i == 0)
    {
      glsl::vec2 const n1 = segments[i].m_leftNormals[StartPoint];
      glsl::vec2 const n2 = segments[i].m_rightNormals[StartPoint];
      glsl::vec2 const n3 = (n1 - kArrowTailFactor * segments[i].m_tangent);
      glsl::vec2 const n4 = (n2 - kArrowTailFactor * segments[i].m_tangent);
      std::vector<glsl::vec2> normals = { n2, n4, n1, n1, n4, n3 };

      m2::RectF t = texRect;
      t.setMaxX(tr.minX());
      std::vector<glsl::vec2> uv =
      {
        glsl::ToVec2(t.RightBottom()),
        glsl::ToVec2(t.LeftBottom()),
        glsl::ToVec2(t.RightTop()),
        glsl::ToVec2(t.RightTop()),
        glsl::ToVec2(t.LeftBottom()),
        glsl::ToVec2(t.LeftTop())
      };

      GenerateArrowsTriangles(startPivot, normals, texRect, uv, false /* normalizedUV */, joinsGeometry);
    }
  }
}

void RouteShape::CacheRouteArrows(ref_ptr<dp::TextureManager> mng, m2::PolylineD const & polyline,
                                  std::vector<ArrowBorders> const & borders, double baseDepthIndex,
                                  SubrouteArrowsData & routeArrowsData)
{
  TArrowGeometryBuffer geometry;
  TArrowGeometryBuffer joinsGeometry;
  dp::TextureManager::SymbolRegion region;
  GetArrowTextureRegion(mng, region);
  auto state = CreateGLState(gpu::ROUTE_ARROW_PROGRAM, RenderState::GeometryLayer);
  state.SetColorTexture(region.GetTexture());

  // Generate arrow geometry.
  auto depth = static_cast<float>(baseDepthIndex * kDepthPerSubroute) + kArrowsDepth;
  float const depthStep = (kArrowsDepth - kRouteDepth) / (1 + borders.size());
  for (ArrowBorders const & b : borders)
  {
    depth -= depthStep;
    std::vector<m2::PointD> points = CalculatePoints(polyline, b.m_startDistance, b.m_endDistance);
    ASSERT_LESS_OR_EQUAL(points.size(), polyline.GetSize(), ());
    PrepareArrowGeometry(points, routeArrowsData.m_pivot, region.GetTexRect(), depthStep,
                         depth, geometry, joinsGeometry);
  }

  BatchGeometry(state, make_ref(geometry.data()), static_cast<uint32_t>(geometry.size()),
                make_ref(joinsGeometry.data()), static_cast<uint32_t>(joinsGeometry.size()),
                AV::GetBindingInfo(), routeArrowsData.m_renderProperty);
}

void RouteShape::CacheRoute(ref_ptr<dp::TextureManager> textures, SubrouteData & subrouteData)
{
  ASSERT_LESS(subrouteData.m_startPointIndex, subrouteData.m_endPointIndex, ());

  auto const points = subrouteData.m_subroute->m_polyline.ExtractSegment(subrouteData.m_startPointIndex,
                                                                         subrouteData.m_endPointIndex);
  if (points.empty())
    return;

  std::vector<glsl::vec4> segmentsColors;
  if (!subrouteData.m_subroute->m_traffic.empty())
  {
    segmentsColors.reserve(subrouteData.m_endPointIndex - subrouteData.m_startPointIndex);
    for (size_t i = subrouteData.m_startPointIndex; i < subrouteData.m_endPointIndex; ++i)
    {
      auto const speedGroup = TrafficGenerator::CheckColorsSimplification(subrouteData.m_subroute->m_traffic[i]);
      auto const colorConstant = TrafficGenerator::GetColorBySpeedGroup(speedGroup, true /* route */);
      dp::Color const color = df::GetColorConstant(colorConstant);
      float const alpha = (speedGroup == traffic::SpeedGroup::G4 ||
        speedGroup == traffic::SpeedGroup::G5 ||
        speedGroup == traffic::SpeedGroup::Unknown) ? 0.0f : 1.0f;
      segmentsColors.emplace_back(color.GetRedF(), color.GetGreenF(), color.GetBlueF(), alpha);
    }
  }

  TGeometryBuffer geometry;
  TGeometryBuffer joinsGeometry;
  PrepareGeometry(points, subrouteData.m_pivot, segmentsColors,
                  static_cast<float>(subrouteData.m_subroute->m_baseDepthIndex * kDepthPerSubroute),
                  geometry, joinsGeometry);

  auto state = CreateGLState(subrouteData.m_subroute->m_style[subrouteData.m_styleIndex].m_pattern.m_isDashed ?
                             gpu::ROUTE_DASH_PROGRAM : gpu::ROUTE_PROGRAM, RenderState::GeometryLayer);
  state.SetColorTexture(textures->GetSymbolsTexture());

  BatchGeometry(state, make_ref(geometry.data()), static_cast<uint32_t>(geometry.size()),
                make_ref(joinsGeometry.data()), static_cast<uint32_t>(joinsGeometry.size()),
                RV::GetBindingInfo(), subrouteData.m_renderProperty);
}

void RouteShape::BatchGeometry(dp::GLState const & state, ref_ptr<void> geometry, uint32_t geomSize,
                               ref_ptr<void> joinsGeometry, uint32_t joinsGeomSize,
                               dp::BindingInfo const & bindingInfo, RouteRenderProperty & property)
{
  size_t const verticesCount = geomSize + joinsGeomSize;
  if (verticesCount == 0)
    return;

  uint32_t const kBatchSize = 5000;
  dp::Batcher batcher(kBatchSize, kBatchSize);
  dp::SessionGuard guard(batcher, [&property](dp::GLState const & state, drape_ptr<dp::RenderBucket> && b)
  {
    property.m_buckets.push_back(std::move(b));
    property.m_state = state;
  });

  if (geomSize != 0)
  {
    dp::AttributeProvider provider(1 /* stream count */, geomSize);
    provider.InitStream(0 /* stream index */, bindingInfo, geometry);
    batcher.InsertListOfStrip(state, make_ref(&provider), 4);
  }

  if (joinsGeomSize != 0)
  {
    dp::AttributeProvider joinsProvider(1 /* stream count */, joinsGeomSize);
    joinsProvider.InitStream(0 /* stream index */, bindingInfo, joinsGeometry);
    batcher.InsertTriangleList(state, make_ref(&joinsProvider));
  }
}
}  // namespace df
