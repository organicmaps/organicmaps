#include "drape_frontend/route_shape.hpp"
#include "drape_frontend/line_shape_helper.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/glsl_func.hpp"
#include "drape/glsl_types.hpp"
#include "drape/shader_def.hpp"
#include "drape/texture_manager.hpp"

#include "base/logging.hpp"

namespace df
{

namespace
{

float const kLeftSide = 1.0;
float const kCenter = 0.0;
float const kRightSide = -1.0;

void GetArrowTextureRegion(ref_ptr<dp::TextureManager> textures, dp::TextureManager::SymbolRegion & region)
{
  textures->GetSymbolRegion("route-arrow", region);
}

vector<m2::PointD> CalculatePoints(m2::PolylineD const & polyline, double start, double end)
{
  vector<m2::PointD> result;
  result.reserve(polyline.GetSize() / 4);

  auto addIfNotExist = [&result](m2::PointD const & pnt)
  {
    if (result.empty() || result.back() != pnt)
      result.push_back(pnt);
  };

  vector<m2::PointD> const & path = polyline.GetPoints();
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

void GenerateJoinsTriangles(glsl::vec3 const & pivot, vector<glsl::vec2> const & normals,
                            glsl::vec2 const & length, bool isLeft, RouteShape::TGeometryBuffer & joinsGeometry)
{
  float const kEps = 1e-5;
  size_t const trianglesCount = normals.size() / 3;
  float const side = isLeft ? kLeftSide : kRightSide;
  for (int j = 0; j < trianglesCount; j++)
  {
    glsl::vec3 const len1 = glsl::vec3(length.x, length.y, glsl::length(normals[3 * j]) < kEps ? kCenter : side);
    glsl::vec3 const len2 = glsl::vec3(length.x, length.y, glsl::length(normals[3 * j + 1]) < kEps ? kCenter : side);
    glsl::vec3 const len3 = glsl::vec3(length.x, length.y, glsl::length(normals[3 * j + 2]) < kEps ? kCenter : side);

    joinsGeometry.push_back(RouteShape::RV(pivot, normals[3 * j], len1));
    joinsGeometry.push_back(RouteShape::RV(pivot, normals[3 * j + 1], len2));
    joinsGeometry.push_back(RouteShape::RV(pivot, normals[3 * j + 2], len3));
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

void GenerateArrowsTriangles(glsl::vec4 const & pivot, vector<glsl::vec2> const & normals,
                             m2::RectF const & texRect, vector<glsl::vec2> const & uv,
                             bool normalizedUV, RouteShape::TArrowGeometryBuffer & joinsGeometry)
{
  size_t const trianglesCount = normals.size() / 3;
  for (int j = 0; j < trianglesCount; j++)
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

void RouteShape::PrepareGeometry(vector<m2::PointD> const & path, TGeometryBuffer & geometry,
                                 TGeometryBuffer & joinsGeometry, double & outputLength)
{
  ASSERT(path.size() > 1, ());

  // Construct segments.
  vector<LineSegment> segments;
  segments.reserve(path.size() - 1);
  ConstructLineSegments(path, segments);

  // Build geometry.
  float length = 0;
  float const kDepth = 0.0f;
  for (size_t i = 0; i < segments.size(); i++)
  {
    UpdateNormals(&segments[i], (i > 0) ? &segments[i - 1] : nullptr,
                 (i < segments.size() - 1) ? &segments[i + 1] : nullptr);

    // Generate main geometry.
    glsl::vec3 const startPivot = glsl::vec3(segments[i].m_points[StartPoint], kDepth);
    glsl::vec3 const endPivot = glsl::vec3(segments[i].m_points[EndPoint], kDepth);

    float const endLength = length + glsl::length(segments[i].m_points[EndPoint] - segments[i].m_points[StartPoint]);

    glsl::vec2 const leftNormalStart = GetNormal(segments[i], true /* isLeft */, StartNormal);
    glsl::vec2 const rightNormalStart = GetNormal(segments[i], false /* isLeft */, StartNormal);
    glsl::vec2 const leftNormalEnd = GetNormal(segments[i], true /* isLeft */, EndNormal);
    glsl::vec2 const rightNormalEnd = GetNormal(segments[i], false /* isLeft */, EndNormal);

    float const projLeftStart = -segments[i].m_leftWidthScalar[StartPoint].y;
    float const projLeftEnd = segments[i].m_leftWidthScalar[EndPoint].y;
    float const projRightStart = -segments[i].m_rightWidthScalar[StartPoint].y;
    float const projRightEnd = segments[i].m_rightWidthScalar[EndPoint].y;

    geometry.push_back(RV(startPivot, glsl::vec2(0, 0), glsl::vec3(length, 0, kCenter)));
    geometry.push_back(RV(startPivot, leftNormalStart, glsl::vec3(length, projLeftStart, kLeftSide)));
    geometry.push_back(RV(endPivot, glsl::vec2(0, 0), glsl::vec3(endLength, 0, kCenter)));
    geometry.push_back(RV(endPivot, leftNormalEnd, glsl::vec3(endLength, projLeftEnd, kLeftSide)));

    geometry.push_back(RV(startPivot, rightNormalStart, glsl::vec3(length, projRightStart, kRightSide)));
    geometry.push_back(RV(startPivot, glsl::vec2(0, 0), glsl::vec3(length, 0, kCenter)));
    geometry.push_back(RV(endPivot, rightNormalEnd, glsl::vec3(endLength, projRightEnd, kRightSide)));
    geometry.push_back(RV(endPivot, glsl::vec2(0, 0), glsl::vec3(endLength, 0, kCenter)));

    // Generate joins.
    if (segments[i].m_generateJoin && i < segments.size() - 1)
    {
      glsl::vec2 n1 = segments[i].m_hasLeftJoin[EndPoint] ? segments[i].m_leftNormals[EndPoint] :
                                                            segments[i].m_rightNormals[EndPoint];
      glsl::vec2 n2 = segments[i + 1].m_hasLeftJoin[StartPoint] ? segments[i + 1].m_leftNormals[StartPoint] :
                                                                  segments[i + 1].m_rightNormals[StartPoint];

      float widthScalar = segments[i].m_hasLeftJoin[EndPoint] ? segments[i].m_rightWidthScalar[EndPoint].x :
                                                                segments[i].m_leftWidthScalar[EndPoint].x;

      vector<glsl::vec2> normals;
      normals.reserve(24);
      GenerateJoinNormals(dp::RoundJoin, n1, n2, 1.0f, segments[i].m_hasLeftJoin[EndPoint], widthScalar, normals);

      GenerateJoinsTriangles(glsl::vec3(segments[i].m_points[EndPoint], kDepth), normals,
                             glsl::vec2(endLength, 0), segments[i].m_hasLeftJoin[EndPoint], joinsGeometry);
    }

    // Generate caps.
    if (i == 0)
    {
      vector<glsl::vec2> normals;
      normals.reserve(24);
      GenerateCapNormals(dp::RoundCap, segments[i].m_leftNormals[StartPoint],
                         segments[i].m_rightNormals[StartPoint], -segments[i].m_tangent,
                         1.0f, true /* isStart */, normals);

      GenerateJoinsTriangles(glsl::vec3(segments[i].m_points[StartPoint], kDepth), normals,
                             glsl::vec2(length, 0), true, joinsGeometry);
    }

    if (i == segments.size() - 1)
    {
      vector<glsl::vec2> normals;
      normals.reserve(24);
      GenerateCapNormals(dp::RoundCap, segments[i].m_leftNormals[EndPoint],
                         segments[i].m_rightNormals[EndPoint], segments[i].m_tangent,
                         1.0f, false /* isStart */, normals);

      GenerateJoinsTriangles(glsl::vec3(segments[i].m_points[EndPoint], kDepth), normals,
                             glsl::vec2(endLength, 0), true, joinsGeometry);
    }

    length = endLength;
  }

  outputLength = length; 
}

void RouteShape::PrepareArrowGeometry(vector<m2::PointD> const & path, m2::RectF const & texRect, float depth,
                                      TArrowGeometryBuffer & geometry, TArrowGeometryBuffer & joinsGeometry)
{
  ASSERT(path.size() > 1, ());

  // Construct segments.
  vector<LineSegment> segments;
  segments.reserve(path.size() - 1);
  ConstructLineSegments(path, segments);

  float finalLength = 0.0f;
  for (size_t i = 0; i < segments.size(); i++)
    finalLength += glsl::length(segments[i].m_points[EndPoint] - segments[i].m_points[StartPoint]);

  m2::RectF tr = texRect;
  tr.setMinX(texRect.minX() * (1.0 - kArrowTailSize) + texRect.maxX() * kArrowTailSize);
  tr.setMaxX(texRect.minX() * kArrowHeadSize + texRect.maxX() * (1.0 - kArrowHeadSize));

  // Build geometry.
  float length = 0;
  for (size_t i = 0; i < segments.size(); i++)
  {
    UpdateNormals(&segments[i], (i > 0) ? &segments[i - 1] : nullptr,
                 (i < segments.size() - 1) ? &segments[i + 1] : nullptr);

    // Generate main geometry.
    glsl::vec4 const startPivot = glsl::vec4(segments[i].m_points[StartPoint], depth, 1.0);
    glsl::vec4 const endPivot = glsl::vec4(segments[i].m_points[EndPoint], depth, 1.0);

    float const endLength = length + glsl::length(segments[i].m_points[EndPoint] - segments[i].m_points[StartPoint]);

    glsl::vec2 const leftNormalStart = GetNormal(segments[i], true /* isLeft */, StartNormal);
    glsl::vec2 const rightNormalStart = GetNormal(segments[i], false /* isLeft */, StartNormal);
    glsl::vec2 const leftNormalEnd = GetNormal(segments[i], true /* isLeft */, EndNormal);
    glsl::vec2 const rightNormalEnd = GetNormal(segments[i], false /* isLeft */, EndNormal);

    float const startU = length / finalLength;
    float const endU = endLength / finalLength;

    geometry.push_back(AV(startPivot, glsl::vec2(0, 0), GetUV(tr, startU, 0.5f)));
    geometry.push_back(AV(startPivot, leftNormalStart, GetUV(tr, startU, 0.0f)));
    geometry.push_back(AV(endPivot, glsl::vec2(0, 0), GetUV(tr, endU, 0.5f)));
    geometry.push_back(AV(endPivot, leftNormalEnd, GetUV(tr, endU, 0.0f)));

    geometry.push_back(AV(startPivot, rightNormalStart, GetUV(tr, startU, 1.0f)));
    geometry.push_back(AV(startPivot, glsl::vec2(0, 0), GetUV(tr, startU, 0.5f)));
    geometry.push_back(AV(endPivot, rightNormalEnd, GetUV(tr, endU, 1.0f)));
    geometry.push_back(AV(endPivot, glsl::vec2(0, 0), GetUV(tr, endU, 0.5f)));

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
      vector<glsl::vec2> normals;
      normals.reserve(kAverageSize);
      vector<glsl::vec2> uv;
      uv.reserve(kAverageSize);

      GenerateJoinNormals(dp::RoundJoin, n1, n2, 1.0f, segments[i].m_hasLeftJoin[EndPoint],
                          widthScalar, normals, &uv);

      ASSERT_EQUAL(normals.size(), uv.size(), ());

      GenerateArrowsTriangles(glsl::vec4(segments[i].m_points[EndPoint], depth, 1.0),
                              normals, tr, uv, true /* normalizedUV */, joinsGeometry);
    }

    // Generate arrow head.
    if (i == segments.size() - 1)
    {
      vector<glsl::vec2> normals =
      {
        segments[i].m_rightNormals[EndPoint],
        segments[i].m_leftNormals[EndPoint],
        kArrowHeadFactor * segments[i].m_tangent
      };
      float const u = 1.0f - kArrowHeadSize;
      vector<glsl::vec2> uv = { glsl::vec2(u, 1.0f), glsl::vec2(u, 0.0f), glsl::vec2(1.0f, 0.5f) };
      GenerateArrowsTriangles(glsl::vec4(segments[i].m_points[EndPoint], depth, 1.0),
                              normals, texRect, uv, true /* normalizedUV */, joinsGeometry);
    }

    // Generate arrow tail.
    if (i == 0)
    {
      glsl::vec2 const n1 = segments[i].m_leftNormals[StartPoint];
      glsl::vec2 const n2 = segments[i].m_rightNormals[StartPoint];
      glsl::vec2 const n3 = (n1 - kArrowTailFactor * segments[i].m_tangent);
      glsl::vec2 const n4 = (n2 - kArrowTailFactor * segments[i].m_tangent);
      vector<glsl::vec2> normals = { n2, n4, n1, n1, n4, n3 };

      m2::RectF t = texRect;
      t.setMaxX(tr.minX());
      vector<glsl::vec2> uv =
      {
        glsl::ToVec2(t.RightBottom()),
        glsl::ToVec2(t.LeftBottom()),
        glsl::ToVec2(t.RightTop()),
        glsl::ToVec2(t.RightTop()),
        glsl::ToVec2(t.LeftBottom()),
        glsl::ToVec2(t.LeftTop())
      };

      GenerateArrowsTriangles(glsl::vec4(segments[i].m_points[StartPoint], depth, 1.0),
                              normals, texRect, uv, false /* normalizedUV */, joinsGeometry);
    }

    length = endLength;
  }
}

void RouteShape::CacheRouteSign(ref_ptr<dp::TextureManager> mng, RouteSignData & routeSignData)
{
  dp::TextureManager::SymbolRegion symbol;
  mng->GetSymbolRegion(routeSignData.m_isStart ? "route_from" : "route_to", symbol);

  m2::RectF const & texRect = symbol.GetTexRect();
  m2::PointF halfSize = m2::PointF(symbol.GetPixelSize()) * 0.5f;

  glsl::vec2 const pos = glsl::ToVec2(routeSignData.m_position);
  glsl::vec4 const pivot = glsl::vec4(pos.x, pos.y, 0.0f /* depth */, 0.0f /* pivot z */);
  gpu::SolidTexturingVertex data[4]=
  {
    { pivot, glsl::vec2(-halfSize.x,  halfSize.y), glsl::ToVec2(texRect.LeftTop()) },
    { pivot, glsl::vec2(-halfSize.x, -halfSize.y), glsl::ToVec2(texRect.LeftBottom()) },
    { pivot, glsl::vec2( halfSize.x,  halfSize.y), glsl::ToVec2(texRect.RightTop()) },
    { pivot, glsl::vec2( halfSize.x, -halfSize.y), glsl::ToVec2(texRect.RightBottom()) }
  };

  dp::GLState state(gpu::TEXTURING_PROGRAM, dp::GLState::OverlayLayer);
  if (!routeSignData.m_isStart)
    state.SetProgram3dIndex(gpu::TEXTURING_BILLBOARD_PROGRAM);
  state.SetColorTexture(symbol.GetTexture());

  {
    dp::Batcher batcher(dp::Batcher::IndexPerQuad, dp::Batcher::VertexPerQuad);
    dp::SessionGuard guard(batcher, [&routeSignData](dp::GLState const & state, drape_ptr<dp::RenderBucket> && b)
    {
      routeSignData.m_sign.m_buckets.push_back(move(b));
      routeSignData.m_sign.m_state = state;
    });

    dp::AttributeProvider provider(1 /* stream count */, dp::Batcher::VertexPerQuad);
    provider.InitStream(0 /* stream index */, gpu::SolidTexturingVertex::GetBindingInfo(), make_ref(data));

    dp::IndicesRange indices = batcher.InsertTriangleStrip(state, make_ref(&provider), nullptr);
    UNUSED_VALUE(indices);
    ASSERT(indices.IsValid(), ());
  }
}

void RouteShape::CacheRouteArrows(ref_ptr<dp::TextureManager> mng, m2::PolylineD const & polyline,
                                  vector<ArrowBorders> const & borders, RouteArrowsData & routeArrowsData)
{
  TArrowGeometryBuffer geometry;
  TArrowGeometryBuffer joinsGeometry;
  dp::TextureManager::SymbolRegion region;
  GetArrowTextureRegion(mng, region);
  dp::GLState state = dp::GLState(gpu::ROUTE_ARROW_PROGRAM, dp::GLState::GeometryLayer);
  state.SetColorTexture(region.GetTexture());

  // Generate arrow geometry.
  float depth = 0.0f;
  for (ArrowBorders const & b : borders)
  {
    vector<m2::PointD> points = CalculatePoints(polyline, b.m_startDistance, b.m_endDistance);
    ASSERT_LESS_OR_EQUAL(points.size(), polyline.GetSize(), ());
    PrepareArrowGeometry(points, region.GetTexRect(), depth, geometry, joinsGeometry);
    depth += 1.0f;
  }

  BatchGeometry(state, make_ref(geometry.data()), geometry.size(),
                make_ref(joinsGeometry.data()), joinsGeometry.size(),
                AV::GetBindingInfo(), routeArrowsData.m_arrows);
}

void RouteShape::CacheRoute(ref_ptr<dp::TextureManager> textures, RouteData & routeData)
{
  TGeometryBuffer geometry;
  TGeometryBuffer joinsGeometry;
  PrepareGeometry(routeData.m_sourcePolyline.GetPoints(),
                  geometry, joinsGeometry, routeData.m_length);

  dp::GLState state = dp::GLState(gpu::ROUTE_PROGRAM, dp::GLState::GeometryLayer);
  state.SetColorTexture(textures->GetSymbolsTexture());
  BatchGeometry(state, make_ref(geometry.data()), geometry.size(),
                make_ref(joinsGeometry.data()), joinsGeometry.size(),
                RV::GetBindingInfo(), routeData.m_route);
}

void RouteShape::BatchGeometry(dp::GLState const & state, ref_ptr<void> geometry, size_t geomSize,
                               ref_ptr<void> joinsGeometry, size_t joinsGeomSize,
                               dp::BindingInfo const & bindingInfo, RouteRenderProperty & property)
{
  size_t const verticesCount = geomSize + joinsGeomSize;
  if (verticesCount != 0)
  {
    uint32_t const kBatchSize = 5000;
    dp::Batcher batcher(kBatchSize, kBatchSize);
    dp::SessionGuard guard(batcher, [&property](dp::GLState const & state, drape_ptr<dp::RenderBucket> && b)
    {
      property.m_buckets.push_back(move(b));
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
}

} // namespace df
