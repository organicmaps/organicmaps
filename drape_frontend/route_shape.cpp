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

float const kArrowsGeometrySegmentLength = 0.5;

void GetArrowTextureRegion(ref_ptr<dp::TextureManager> textures, dp::TextureManager::SymbolRegion & region)
{
  textures->GetSymbolRegion("route-arrow", region);
}

void ClipArrowToSegments(vector<double> const & turns, RouteData & routeData)
{
  int const cnt = static_cast<int>(routeData.m_length / kArrowsGeometrySegmentLength) + 1;
  routeData.m_arrows.reserve(cnt);
  for (int i = 0; i < cnt; ++i)
  {
    double const start = i * kArrowsGeometrySegmentLength;
    double const end = (i + 1) * kArrowsGeometrySegmentLength;

    drape_ptr<ArrowRenderProperty> arrowRenderProperty = make_unique_dp<ArrowRenderProperty>();

    // looking for corresponding turns
    int startTurnIndex = -1;
    int endTurnIndex = -1;
    for (size_t j = 0; j < turns.size(); ++j)
    {
      if (turns[j] >= start && turns[j] < end)
      {
        if (startTurnIndex < 0)
          startTurnIndex = j;

        if (startTurnIndex >= 0)
          endTurnIndex = j;

        arrowRenderProperty->m_turns.push_back(turns[j]);
      }
    }

    if (startTurnIndex < 0 || endTurnIndex < 0)
      continue;

    // start of arrow segment
    if (startTurnIndex != 0)
    {
      double d = max(0.5 * (turns[startTurnIndex] + turns[startTurnIndex - 1]),
                     turns[startTurnIndex] - kArrowSize);
      arrowRenderProperty->m_start = max(0.0, d);
    }
    else
    {
      arrowRenderProperty->m_start = max(0.0, turns[startTurnIndex] - kArrowSize);
    }

    // end of arrow segment
    if (endTurnIndex + 1 != turns.size())
    {
      double d = min(0.5 * (turns[endTurnIndex] + turns[endTurnIndex + 1]),
                     turns[endTurnIndex] + kArrowSize);
      arrowRenderProperty->m_end = min(routeData.m_length, d);
    }
    else
    {
      arrowRenderProperty->m_end = min(routeData.m_length, turns[endTurnIndex] + kArrowSize);
    }

    // rescale turns
    for (size_t j = 0; j < arrowRenderProperty->m_turns.size(); ++j)
      arrowRenderProperty->m_turns[j] -= arrowRenderProperty->m_start;

    routeData.m_arrows.push_back(move(arrowRenderProperty));
  }
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
    double dist = (path[i + 1] - path[i]).Length();
    if (fabs(dist) < 1e-5)
      continue;

    double l = len + dist;
    if (!started && start >= len && start <= l)
    {
      double k = (start - len) / dist;
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
      double k = (end - len) / dist;
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

}

RouteShape::RouteShape(m2::PolylineD const & polyline,  vector<double> const & turns,
                       CommonViewParams const & params)
  : m_params(params)
  , m_polyline(polyline)
  , m_turns(turns)
{}

void RouteShape::PrepareGeometry(bool isRoute, vector<m2::PointD> const & path,
                                 TGeometryBuffer & geometry, TGeometryBuffer & joinsGeometry,
                                 vector<RouteJoinBounds> & joinsBounds, double & outputLength)
{
  ASSERT(path.size() > 1, ());

  auto const generateTriangles = [&joinsGeometry](glsl::vec3 const & pivot, vector<glsl::vec2> const & normals,
                                                  glsl::vec2 const & length, bool isLeft)
  {
    float const eps = 1e-5;
    size_t const trianglesCount = normals.size() / 3;
    float const side = isLeft ? kLeftSide : kRightSide;
    for (int j = 0; j < trianglesCount; j++)
    {
      glsl::vec3 const len1 = glsl::vec3(length.x, length.y, glsl::length(normals[3 * j]) < eps ? kCenter : side);
      glsl::vec3 const len2 = glsl::vec3(length.x, length.y, glsl::length(normals[3 * j + 1]) < eps ? kCenter : side);
      glsl::vec3 const len3 = glsl::vec3(length.x, length.y, glsl::length(normals[3 * j + 2]) < eps ? kCenter : side);

      joinsGeometry.push_back(RV(pivot, normals[3 * j], len1));
      joinsGeometry.push_back(RV(pivot, normals[3 * j + 1], len2));
      joinsGeometry.push_back(RV(pivot, normals[3 * j + 2], len3));
    }
  };

  // constuct segments
  vector<LineSegment> segments;
  segments.reserve(path.size() - 1);
  ConstructLineSegments(path, segments);

  // build geometry
  float length = 0;
  for (size_t i = 0; i < segments.size(); i++)
  {
    UpdateNormals(&segments[i], (i > 0) ? &segments[i - 1] : nullptr,
                 (i < segments.size() - 1) ? &segments[i + 1] : nullptr);

    // generate main geometry
    glsl::vec3 const startPivot = glsl::vec3(segments[i].m_points[StartPoint], m_params.m_depth);
    glsl::vec3 const endPivot = glsl::vec3(segments[i].m_points[EndPoint], m_params.m_depth);

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

    // generate joins
    if (i < segments.size() - 1)
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

      generateTriangles(glsl::vec3(segments[i].m_points[EndPoint], m_params.m_depth), normals,
                        glsl::vec2(endLength, 0), segments[i].m_hasLeftJoin[EndPoint]);
    }

    // generate caps
    if (isRoute && i == 0)
    {
      vector<glsl::vec2> normals;
      normals.reserve(24);
      GenerateCapNormals(dp::RoundCap, segments[i].m_leftNormals[StartPoint],
                         segments[i].m_rightNormals[StartPoint], -segments[i].m_tangent,
                         1.0f, true /* isStart */, normals);

      generateTriangles(glsl::vec3(segments[i].m_points[StartPoint], m_params.m_depth), normals,
                        glsl::vec2(length, 0), true);
    }

    if (isRoute && i == segments.size() - 1)
    {
      vector<glsl::vec2> normals;
      normals.reserve(24);
      GenerateCapNormals(dp::RoundCap, segments[i].m_leftNormals[EndPoint],
                         segments[i].m_rightNormals[EndPoint], segments[i].m_tangent,
                         1.0f, false /* isStart */, normals);

      generateTriangles(glsl::vec3(segments[i].m_points[EndPoint], m_params.m_depth), normals,
                        glsl::vec2(endLength, 0), true);
    }

    length = endLength;
  }

  outputLength = length;

  // calculate joins bounds
  if (!isRoute)
  {
    float const eps = 1e-5;
    double len = 0;
    for (size_t i = 0; i < segments.size() - 1; i++)
    {
      len += glsl::length(segments[i].m_points[EndPoint] - segments[i].m_points[StartPoint]);

      RouteJoinBounds bounds;
      bounds.m_start = min(segments[i].m_leftWidthScalar[EndPoint].y,
                           segments[i].m_rightWidthScalar[EndPoint].y);
      bounds.m_end = max(-segments[i + 1].m_leftWidthScalar[StartPoint].y,
                         -segments[i + 1].m_rightWidthScalar[StartPoint].y);

      if (fabs(bounds.m_end - bounds.m_start) < eps)
        continue;

      bounds.m_offset = len;
      joinsBounds.push_back(bounds);
    }
  }
}

void RouteShape::CacheEndOfRouteSign(ref_ptr<dp::TextureManager> mng, RouteData & routeData)
{
  dp::TextureManager::SymbolRegion symbol;
  mng->GetSymbolRegion("route_to", symbol);

  m2::RectF const & texRect = symbol.GetTexRect();
  m2::PointF halfSize = m2::PointF(symbol.GetPixelSize()) * 0.5f;

  glsl::vec2 const pos = glsl::ToVec2(m_polyline.Back());
  glsl::vec3 const pivot = glsl::vec3(pos.x, pos.y, m_params.m_depth);
  gpu::SolidTexturingVertex data[4]=
  {
    { pivot, glsl::vec2(-halfSize.x,  halfSize.y), glsl::ToVec2(texRect.LeftTop()) },
    { pivot, glsl::vec2(-halfSize.x, -halfSize.y), glsl::ToVec2(texRect.LeftBottom()) },
    { pivot, glsl::vec2( halfSize.x,  halfSize.y), glsl::ToVec2(texRect.RightTop()) },
    { pivot, glsl::vec2( halfSize.x, -halfSize.y), glsl::ToVec2(texRect.RightBottom()) }
  };

  dp::GLState state(gpu::TEXTURING_PROGRAM, dp::GLState::OverlayLayer);
  state.SetColorTexture(symbol.GetTexture());

  {
    dp::Batcher batcher(dp::Batcher::IndexPerQuad, dp::Batcher::VertexPerQuad);
    dp::SessionGuard guard(batcher, [&routeData](dp::GLState const & state, drape_ptr<dp::RenderBucket> && b)
    {
      routeData.m_endOfRouteSign.m_buckets.push_back(move(b));
      routeData.m_endOfRouteSign.m_state = state;
    });

    dp::AttributeProvider provider(1 /* stream count */, dp::Batcher::VertexPerQuad);
    provider.InitStream(0 /* stream index */, gpu::SolidTexturingVertex::GetBindingInfo(), make_ref(data));

    dp::IndicesRange indices = batcher.InsertTriangleStrip(state, make_ref(&provider), nullptr);
    ASSERT(indices.IsValid(), ());
  }
}

void RouteShape::Draw(ref_ptr<dp::TextureManager> textures, RouteData & routeData)
{
  // route geometry
  {
    TGeometryBuffer geometry;
    TGeometryBuffer joinsGeometry;
    vector<RouteJoinBounds> bounds;
    PrepareGeometry(true /* isRoute */, m_polyline.GetPoints(), geometry, joinsGeometry, bounds, routeData.m_length);

    dp::GLState state = dp::GLState(gpu::ROUTE_PROGRAM, dp::GLState::GeometryLayer);
    BatchGeometry(state, geometry, joinsGeometry, routeData.m_route);
  }

  // arrows geometry
  if (!m_turns.empty())
  {
    dp::TextureManager::SymbolRegion region;
    GetArrowTextureRegion(textures, region);
    routeData.m_arrowTextureRect = region.GetTexRect();

    dp::GLState state = dp::GLState(gpu::ROUTE_ARROW_PROGRAM, dp::GLState::GeometryLayer);
    state.SetColorTexture(region.GetTexture());

    ClipArrowToSegments(m_turns, routeData);
    for (auto & renderProperty : routeData.m_arrows)
    {
      TGeometryBuffer geometry;
      TGeometryBuffer joinsGeometry;
      vector<m2::PointD> points = CalculatePoints(m_polyline, renderProperty->m_start, renderProperty->m_end);
      ASSERT_LESS_OR_EQUAL(points.size(), m_polyline.GetSize(), ());
      PrepareGeometry(false /* isRoute */, points, geometry, joinsGeometry, renderProperty->m_joinsBounds, routeData.m_length);
      BatchGeometry(state, geometry, joinsGeometry, renderProperty->m_arrow);
    }
  }

  // end of route sign
  CacheEndOfRouteSign(textures, routeData);
}

void RouteShape::BatchGeometry(dp::GLState const & state, TGeometryBuffer & geometry,
                               TGeometryBuffer & joinsGeometry, RouteRenderProperty & property)
{
  size_t const verticesCount = geometry.size() + joinsGeometry.size();
  if (verticesCount != 0)
  {
    dp::Batcher batcher(verticesCount, verticesCount);
    dp::SessionGuard guard(batcher, [&property](dp::GLState const & state, drape_ptr<dp::RenderBucket> && b)
    {
      property.m_buckets.push_back(move(b));
      property.m_state = state;
    });

    dp::AttributeProvider provider(1 /* stream count */, geometry.size());
    provider.InitStream(0 /* stream index */, gpu::RouteVertex::GetBindingInfo(), make_ref(geometry.data()));
    batcher.InsertListOfStrip(state, make_ref(&provider), 4);

    if (!joinsGeometry.empty())
    {
      dp::AttributeProvider joinsProvider(1 /* stream count */, joinsGeometry.size());
      joinsProvider.InitStream(0 /* stream index */, gpu::RouteVertex::GetBindingInfo(), make_ref(joinsGeometry.data()));
      batcher.InsertTriangleList(state, make_ref(&joinsProvider));
    }
  }
}

} // namespace df
