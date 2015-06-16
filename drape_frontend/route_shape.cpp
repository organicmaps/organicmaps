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
  float const LEFT_SIDE = 1.0;
  float const CENTER = 0.0;
  float const RIGHT_SIDE = -1.0;

  void GetArrowTextureRegion(ref_ptr<dp::TextureManager> textures, dp::TextureManager::SymbolRegion & region)
  {
    textures->GetSymbolRegion("route-arrow", region);
  }
}

RouteShape::RouteShape(m2::PolylineD const & polyline, CommonViewParams const & params)
  : m_params(params)
  , m_polyline(polyline)
  , m_length(0)
  , m_endOfRouteState(0, dp::GLState::OverlayLayer)
{}

m2::RectF RouteShape::GetArrowTextureRect(ref_ptr<dp::TextureManager> textures) const
{
  dp::TextureManager::SymbolRegion region;
  GetArrowTextureRegion(textures, region);
  return region.GetTexRect();
}

void RouteShape::PrepareGeometry(ref_ptr<dp::TextureManager> textures)
{
  vector<m2::PointD> const & path = m_polyline.GetPoints();
  ASSERT(path.size() > 1, ());

  auto const generateTriangles = [&](glsl::vec3 const & pivot, vector<glsl::vec2> const & normals,
                                     glsl::vec2 const & length, bool isLeft)
  {
    float const eps = 1e-5;
    size_t const trianglesCount = normals.size() / 3;
    float const side = isLeft ? LEFT_SIDE : RIGHT_SIDE;
    for (int j = 0; j < trianglesCount; j++)
    {
      glsl::vec3 const len1 = glsl::vec3(length.x, length.y, glsl::length(normals[3 * j]) < eps ? CENTER : side);
      glsl::vec3 const len2 = glsl::vec3(length.x, length.y, glsl::length(normals[3 * j + 1]) < eps ? CENTER : side);
      glsl::vec3 const len3 = glsl::vec3(length.x, length.y, glsl::length(normals[3 * j + 2]) < eps ? CENTER : side);

      m_joinsGeometry.push_back(RV(pivot, normals[3 * j], len1));
      m_joinsGeometry.push_back(RV(pivot, normals[3 * j + 1], len2));
      m_joinsGeometry.push_back(RV(pivot, normals[3 * j + 2], len3));
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

    m_geometry.push_back(RV(startPivot, glsl::vec2(0, 0), glsl::vec3(length, 0, CENTER)));
    m_geometry.push_back(RV(startPivot, leftNormalStart, glsl::vec3(length, projLeftStart, LEFT_SIDE)));
    m_geometry.push_back(RV(endPivot, glsl::vec2(0, 0), glsl::vec3(endLength, 0, CENTER)));
    m_geometry.push_back(RV(endPivot, leftNormalEnd, glsl::vec3(endLength, projLeftEnd, LEFT_SIDE)));

    m_geometry.push_back(RV(startPivot, rightNormalStart, glsl::vec3(length, projRightStart, RIGHT_SIDE)));
    m_geometry.push_back(RV(startPivot, glsl::vec2(0, 0), glsl::vec3(length, 0, CENTER)));
    m_geometry.push_back(RV(endPivot, rightNormalEnd, glsl::vec3(endLength, projRightEnd, RIGHT_SIDE)));
    m_geometry.push_back(RV(endPivot, glsl::vec2(0, 0), glsl::vec3(endLength, 0, CENTER)));

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
    if (i == 0)
    {
      vector<glsl::vec2> normals;
      normals.reserve(24);
      GenerateCapNormals(dp::RoundCap, segments[i].m_leftNormals[StartPoint],
                         segments[i].m_rightNormals[StartPoint], -segments[i].m_tangent,
                         1.0f, true /* isStart */, normals);

      generateTriangles(glsl::vec3(segments[i].m_points[StartPoint], m_params.m_depth), normals,
                        glsl::vec2(length, 0), true);
    }

    if (i == segments.size() - 1)
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

  m_length = length;

  // calculate joins bounds
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
    m_joinsBounds.push_back(bounds);
  }

  CacheEndOfRouteSign(textures);
}

void RouteShape::CacheEndOfRouteSign(ref_ptr<dp::TextureManager> mng)
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
    dp::SessionGuard guard(batcher, [this](dp::GLState const & state, drape_ptr<dp::RenderBucket> && b)
    {
      m_endOfRouteRenderBucket = move(b);
      m_endOfRouteState = state;
    });

    dp::AttributeProvider provider(1 /*stream count*/, dp::Batcher::VertexPerQuad);
    provider.InitStream(0 /*stream index*/, gpu::SolidTexturingVertex::GetBindingInfo(), make_ref(data));

    dp::IndicesRange indices = batcher.InsertTriangleStrip(state, make_ref(&provider), nullptr);
    ASSERT(indices.IsValid(), ());
  }
}

void RouteShape::Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures)
{
  ASSERT(!m_geometry.empty(), ());

  dp::TextureManager::SymbolRegion region;
  GetArrowTextureRegion(textures, region);

  dp::GLState state(gpu::ROUTE_PROGRAM, dp::GLState::GeometryLayer);
  state.SetColorTexture(region.GetTexture());

  dp::AttributeProvider provider(1, m_geometry.size());
  provider.InitStream(0, gpu::RouteVertex::GetBindingInfo(), make_ref(m_geometry.data()));
  batcher->InsertListOfStrip(state, make_ref(&provider), 4);

  if (!m_joinsGeometry.empty())
  {
    dp::AttributeProvider joinsProvider(1, m_joinsGeometry.size());
    joinsProvider.InitStream(0, gpu::RouteVertex::GetBindingInfo(), make_ref(m_joinsGeometry.data()));
    batcher->InsertTriangleList(state, make_ref(&joinsProvider));
  }
}

} // namespace df
