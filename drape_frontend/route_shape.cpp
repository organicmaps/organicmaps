#include "drape_frontend/route_shape.hpp"

#include "drape_frontend/line_shape_helper.hpp"

#include "drape/utils/vertex_decl.hpp"
#include "drape/glsl_types.hpp"
#include "drape/glsl_func.hpp"
#include "drape/shader_def.hpp"
#include "drape/attribute_provider.hpp"
#include "drape/glstate.hpp"
#include "drape/batcher.hpp"

#include "base/logging.hpp"

namespace df
{

namespace
{
  using RV = gpu::RouteVertex;
  using TGeometryBuffer = buffer_vector<gpu::RouteVertex, 128>;
}

RouteShape::RouteShape(m2::PolylineD const & polyline, CommonViewParams const & params)
  : m_params(params)
  , m_polyline(polyline)
{}

void RouteShape::Draw(ref_ptr<dp::Batcher> batcher) const
{
  TGeometryBuffer geometry;
  TGeometryBuffer joinsGeometry;
  vector<m2::PointD> const & path = m_polyline.GetPoints();
  ASSERT(path.size() > 1, ());

  auto const generateTriangles = [&](glsl::vec3 const & pivot, vector<glsl::vec2> const & normals,
                                     glsl::vec2 const & length)
  {
    size_t const trianglesCount = normals.size() / 3;
    for (int j = 0; j < trianglesCount; j++)
    {
      joinsGeometry.push_back(RV(pivot, normals[3 * j], length));
      joinsGeometry.push_back(RV(pivot, normals[3 * j + 1], length));
      joinsGeometry.push_back(RV(pivot, normals[3 * j + 2], length));
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

    geometry.push_back(RV(startPivot, glsl::vec2(0, 0), glsl::vec2(length, 0)));
    geometry.push_back(RV(startPivot, leftNormalStart, glsl::vec2(length, projLeftStart)));
    geometry.push_back(RV(endPivot, glsl::vec2(0, 0), glsl::vec2(endLength, 0)));
    geometry.push_back(RV(endPivot, leftNormalEnd, glsl::vec2(endLength, projLeftEnd)));

    geometry.push_back(RV(startPivot, rightNormalStart, glsl::vec2(length, projRightStart)));
    geometry.push_back(RV(startPivot, glsl::vec2(0, 0), glsl::vec2(length, 0)));
    geometry.push_back(RV(endPivot, rightNormalEnd, glsl::vec2(endLength, projRightEnd)));
    geometry.push_back(RV(endPivot, glsl::vec2(0, 0), glsl::vec2(endLength, 0)));

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

      generateTriangles(glsl::vec3(segments[i].m_points[EndPoint], m_params.m_depth), normals, glsl::vec2(endLength, 0));
    }

    // generate caps
    if (i == 0)
    {
      vector<glsl::vec2> normals;
      normals.reserve(24);
      GenerateCapNormals(dp::RoundCap, segments[i].m_leftNormals[StartPoint],
                         segments[i].m_rightNormals[StartPoint], -segments[i].m_tangent,
                         1.0f, true /* isStart */, normals);

      generateTriangles(glsl::vec3(segments[i].m_points[StartPoint], m_params.m_depth), normals, glsl::vec2(length, 0));
    }

    if (i == segments.size() - 1)
    {
      vector<glsl::vec2> normals;
      normals.reserve(24);
      GenerateCapNormals(dp::RoundCap, segments[i].m_leftNormals[EndPoint],
                         segments[i].m_rightNormals[EndPoint], segments[i].m_tangent,
                         1.0f, false /* isStart */, normals);

      generateTriangles(glsl::vec3(segments[i].m_points[EndPoint], m_params.m_depth), normals, glsl::vec2(endLength, 0));
    }

    length = endLength;
  }

  dp::GLState state(gpu::ROUTE_PROGRAM, dp::GLState::GeometryLayer);

  dp::AttributeProvider provider(1, geometry.size());
  provider.InitStream(0, gpu::RouteVertex::GetBindingInfo(), make_ref(geometry.data()));
  batcher->InsertListOfStrip(state, make_ref(&provider), 4);

  dp::AttributeProvider joinsProvider(1, joinsGeometry.size());
  joinsProvider.InitStream(0, gpu::RouteVertex::GetBindingInfo(), make_ref(joinsGeometry.data()));
  batcher->InsertTriangleList(state, make_ref(&joinsProvider));
}

} // namespace df
