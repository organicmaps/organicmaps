#include "drape_frontend/line_shape.hpp"

#include "drape/utils/vertex_decl.hpp"
#include "drape/glsl_types.hpp"
#include "drape/glsl_func.hpp"
#include "drape/shader_def.hpp"
#include "drape/attribute_provider.hpp"
#include "drape/glstate.hpp"
#include "drape/batcher.hpp"
#include "drape/texture_manager.hpp"

#include "base/logging.hpp"

namespace df
{

namespace
{
  using LV = gpu::LineVertex;
  using TGeometryBuffer = buffer_vector<gpu::LineVertex, 128>;

  class TextureCoordGenerator
  {
  public:
    TextureCoordGenerator(float const baseGtoPScale)
      : m_baseGtoPScale(baseGtoPScale)
    {
    }

    void SetRegion(dp::TextureManager::StippleRegion const & region, bool isSolid)
    {
      m_isSolid = isSolid;
      m_region = region;
      if (!m_isSolid)
        m_maskLength = static_cast<float>(m_region.GetMaskPixelLength());
    }

    float GetUvOffsetByDistance(float distance) const
    {
      return m_isSolid ? 1.0 : (distance * m_baseGtoPScale / m_maskLength);
    }

    glsl::vec2 GetTexCoordsByDistance(float distance) const
    {
      float normalizedOffset = min(GetUvOffsetByDistance(distance), 1.0f);
      return GetTexCoords(normalizedOffset);
    }

    glsl::vec2 GetTexCoords(float normalizedOffset) const
    {
      m2::RectF const & texRect = m_region.GetTexRect();
      if (m_isSolid)
        return glsl::ToVec2(texRect.Center());

      return glsl::vec2(texRect.minX() + normalizedOffset * texRect.SizeX(), texRect.Center().y);
    }

    float GetMaskLength() const { return m_maskLength; }
    bool IsSolid() const { return m_isSolid; }

  private:
    float const m_baseGtoPScale;
    dp::TextureManager::StippleRegion m_region;
    float m_maskLength = 0.0f;
    bool m_isSolid = true;
  };

  enum EPointType
  {
    StartPoint = 0,
    EndPoint = 1,
    PointsCount = 2
  };

  enum ENormalType
  {
    StartNormal = 0,
    EndNormal = 1,
    BaseNormal = 2
  };

  struct LineSegment
  {
    glsl::vec2 m_points[PointsCount];
    glsl::vec2 m_tangent;
    glsl::vec2 m_leftBaseNormal;
    glsl::vec2 m_leftNormals[PointsCount];
    glsl::vec2 m_rightBaseNormal;
    glsl::vec2 m_rightNormals[PointsCount];
    glsl::vec2 m_leftWidthScalar[PointsCount];
    glsl::vec2 m_rightWidthScalar[PointsCount];
    bool m_hasLeftJoin[PointsCount];

    LineSegment()
    {
      m_leftWidthScalar[StartPoint] = m_leftWidthScalar[EndPoint] = glsl::vec2(1.0f, 0.0f);
      m_rightWidthScalar[StartPoint] = m_rightWidthScalar[EndPoint] = glsl::vec2(1.0f, 0.0f);
      m_hasLeftJoin[StartPoint] = m_hasLeftJoin[EndPoint] = true;
    }
  };

  void UpdateNormalBetweenSegments(LineSegment * segment1, LineSegment * segment2)
  {
    ASSERT(segment1 != nullptr && segment2 != nullptr, ());

    float const dotProduct = glsl::dot(segment1->m_leftNormals[EndPoint],
                                       segment2->m_leftNormals[StartPoint]);
    float const absDotProduct = fabs(dotProduct);
    float const eps = 1e-5;

    if (fabs(absDotProduct - 1.0f) < eps)
    {
      // change nothing
      return;
    }

    float const crossProduct = glsl::cross(glsl::vec3(segment1->m_tangent, 0),
                                           glsl::vec3(segment2->m_tangent, 0)).z;
    if (crossProduct < 0)
    {
      segment1->m_hasLeftJoin[EndPoint] = true;
      segment2->m_hasLeftJoin[StartPoint] = true;

      // change right-side normals
      glsl::vec2 averageNormal = glsl::normalize(segment1->m_rightNormals[EndPoint] +
                                                 segment2->m_rightNormals[StartPoint]);
      segment1->m_rightNormals[EndPoint] = averageNormal;
      segment2->m_rightNormals[StartPoint] = averageNormal;

      float const cosAngle = glsl::dot(segment1->m_tangent, averageNormal);
      segment1->m_rightWidthScalar[EndPoint].x = 1.0f / sqrt(1.0f - cosAngle * cosAngle);
      segment1->m_rightWidthScalar[EndPoint].y = segment1->m_rightWidthScalar[EndPoint].x * cosAngle;
      segment2->m_rightWidthScalar[StartPoint] = segment1->m_rightWidthScalar[EndPoint];
    }
    else
    {
      segment1->m_hasLeftJoin[EndPoint] = false;
      segment2->m_hasLeftJoin[StartPoint] = false;

      // change left-side normals
      glsl::vec2 averageNormal = glsl::normalize(segment1->m_leftNormals[EndPoint] +
                                                 segment2->m_leftNormals[StartPoint]);
      segment1->m_leftNormals[EndPoint] = averageNormal;
      segment2->m_leftNormals[StartPoint] = averageNormal;

      float const cosAngle = glsl::dot(segment1->m_tangent, averageNormal);
      segment1->m_leftWidthScalar[EndPoint].x = 1.0f / sqrt(1.0f - cosAngle * cosAngle);
      segment1->m_leftWidthScalar[EndPoint].y = segment1->m_leftWidthScalar[EndPoint].x * cosAngle;
      segment2->m_leftWidthScalar[StartPoint] = segment1->m_leftWidthScalar[EndPoint];
    }
  }

  void UpdateNormals(LineSegment * segment, LineSegment * prevSegment, LineSegment * nextSegment)
  {
    ASSERT(segment != nullptr, ());

    if (prevSegment != nullptr)
      UpdateNormalBetweenSegments(prevSegment, segment);

    if (nextSegment != nullptr)
      UpdateNormalBetweenSegments(segment, nextSegment);
  }

  void GenerateJoinNormals(dp::LineJoin joinType, glsl::vec2 const & normal1, glsl::vec2 const & normal2,
                           float halfWidth, bool isLeft, float widthScalar, vector<glsl::vec2> & normals)
  {
    if (joinType == dp::LineJoin::BevelJoin)
    {
      glsl::vec2 const n1 = halfWidth * normal1;
      glsl::vec2 const n2 = halfWidth * normal2;

      normals.push_back(glsl::vec2(0.0f, 0.0f));
      normals.push_back(isLeft ? n1 : n2);
      normals.push_back(isLeft ? n2 : n1);
    }
    else if (joinType == dp::LineJoin::MiterJoin)
    {
      glsl::vec2 averageNormal = halfWidth * widthScalar * glsl::normalize(normal1 + normal2);

      glsl::vec2 const n1 = halfWidth * normal1;
      glsl::vec2 const n2 = halfWidth * normal2;

      normals.push_back(glsl::vec2(0.0f, 0.0f));
      normals.push_back(isLeft ? n1 : averageNormal);
      normals.push_back(isLeft ? averageNormal : n1);

      normals.push_back(glsl::vec2(0.0f, 0.0f));
      normals.push_back(isLeft ? averageNormal : n2);
      normals.push_back(isLeft ? n2 : averageNormal);
    }
    else
    {
      double const segmentAngle = math::pi / 8.0;
      double const fullAngle = acos(glsl::dot(normal1, normal2));
      int segmentsCount = static_cast<int>(fullAngle / segmentAngle);
      if (segmentsCount == 0)
        segmentsCount = 1;

      double const angle = fullAngle / segmentsCount * (isLeft ? -1.0 : 1.0);
      glsl::vec2 const normalizedNormal = glsl::normalize(normal1);
      m2::PointD const startNormal(normalizedNormal.x, normalizedNormal.y);

      for (int i = 0; i < segmentsCount; i++)
      {
        m2::PointD n1 = m2::Rotate(startNormal, i * angle) * halfWidth;
        m2::PointD n2 = m2::Rotate(startNormal, (i + 1) * angle) * halfWidth;

        normals.push_back(glsl::vec2(0.0f, 0.0f));
        normals.push_back(isLeft ? glsl::vec2(n1.x, n1.y) : glsl::vec2(n2.x, n2.y));
        normals.push_back(isLeft ? glsl::vec2(n2.x, n2.y) : glsl::vec2(n1.x, n1.y));
      }
    }
  }

  void GenerateCapNormals(dp::LineCap capType, glsl::vec2 const & normal1, glsl::vec2 const & normal2,
                          glsl::vec2 const & direction, float halfWidth, bool isStart, vector<glsl::vec2> & normals)
  {
    if (capType == dp::ButtCap)
      return;

    if (capType == dp::SquareCap)
    {
      glsl::vec2 const n1 = halfWidth * normal1;
      glsl::vec2 const n2 = halfWidth * normal2;
      glsl::vec2 const n3 = halfWidth * (normal1 + direction);
      glsl::vec2 const n4 = halfWidth * (normal2 + direction);

      normals.push_back(n2);
      normals.push_back(isStart ? n4 : n1);
      normals.push_back(isStart ? n1 : n4);

      normals.push_back(n1);
      normals.push_back(isStart ? n4 : n3);
      normals.push_back(isStart ? n3 : n4);
    }
    else
    {
      int const segmentsCount = 8;
      double const segmentSize = math::pi / segmentsCount * (isStart ? -1.0 : 1.0);
      glsl::vec2 const normalizedNormal = glsl::normalize(normal2);
      m2::PointD const startNormal(normalizedNormal.x, normalizedNormal.y);

      for (int i = 0; i < segmentsCount; i++)
      {
        m2::PointD n1 = m2::Rotate(startNormal, i * segmentSize) * halfWidth;
        m2::PointD n2 = m2::Rotate(startNormal, (i + 1) * segmentSize) * halfWidth;

        normals.push_back(glsl::vec2(0.0f, 0.0f));
        normals.push_back(isStart ? glsl::vec2(n1.x, n1.y) : glsl::vec2(n2.x, n2.y));
        normals.push_back(isStart ? glsl::vec2(n2.x, n2.y) : glsl::vec2(n1.x, n1.y));
      }
    }
  }

  float GetProjectionLength(glsl::vec2 const & newPoint, glsl::vec2 const & startPoint,
                            glsl::vec2 const & endPoint)
  {
    glsl::vec2 const v1 = endPoint - startPoint;
    glsl::vec2 const v2 = newPoint - startPoint;
    float const squareLen = glsl::dot(v1, v1);
    float const proj = glsl::dot(v1, v2) / squareLen;
    return sqrt(squareLen) * my::clamp(proj, 0.0f, 1.0f);
  }

  void CalculateTangentAndNormals(glsl::vec2 const & pt0, glsl::vec2 const & pt1,
                                  glsl::vec2 & tangent, glsl::vec2 & leftNormal,
                                  glsl::vec2 & rightNormal)
  {
    tangent = glsl::normalize(pt1 - pt0);
    leftNormal = glsl::vec2(-tangent.y, tangent.x);
    rightNormal = -leftNormal;
  }

  glsl::vec2 GetNormal(LineSegment const & segment, bool isLeft, ENormalType normalType)
  {
    if (normalType == BaseNormal)
      return isLeft ? segment.m_leftBaseNormal : segment.m_rightBaseNormal;

    int const index = (normalType == StartNormal) ? StartPoint : EndPoint;
    return isLeft ? segment.m_leftWidthScalar[index].x * segment.m_leftNormals[index]:
                    segment.m_rightWidthScalar[index].x  * segment.m_rightNormals[index];
  }
}

LineShape::LineShape(m2::SharedSpline const & spline,
                     LineViewParams const & params)
  : m_params(params)
  , m_spline(spline)
{
  ASSERT_GREATER(m_spline->GetPath().size(), 1, ());
}

void LineShape::Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const
{
  TGeometryBuffer geometry;
  TGeometryBuffer joinsGeometry;
  vector<m2::PointD> const & path = m_spline->GetPath();
  ASSERT(path.size() > 1, ());

  // set up uv-generator
  dp::TextureManager::ColorRegion colorRegion;
  textures->GetColorRegion(m_params.m_color, colorRegion);
  glsl::vec2 colorCoord(glsl::ToVec2(colorRegion.GetTexRect().Center()));

  TextureCoordGenerator texCoordGen(m_params.m_baseGtoPScale);
  dp::TextureManager::StippleRegion maskRegion;
  if (m_params.m_pattern.empty())
    textures->GetStippleRegion(dp::TextureManager::TStipplePattern{1}, maskRegion);
  else
    textures->GetStippleRegion(m_params.m_pattern, maskRegion);

  texCoordGen.SetRegion(maskRegion, m_params.m_pattern.empty());

  float const halfWidth = m_params.m_width / 2.0f;
  float const glbHalfWidth = halfWidth / m_params.m_baseGtoPScale;

  auto const getLineVertex = [&](LineSegment const & segment, glsl::vec3 const & pivot,
                                 glsl::vec2 const & normal, float offsetFromStart)
  {
    float distance = GetProjectionLength(pivot.xy() + glbHalfWidth * normal,
                                         segment.m_points[StartPoint],
                                         segment.m_points[EndPoint]) - offsetFromStart;

    return LV(pivot, halfWidth * normal, colorCoord, texCoordGen.GetTexCoordsByDistance(distance));
  };

  auto const generateTriangles = [&](glsl::vec3 const & pivot, vector<glsl::vec2> const & normals)
  {
    size_t const trianglesCount = normals.size() / 3;
    for (int j = 0; j < trianglesCount; j++)
    {
      glsl::vec2 const uv = texCoordGen.GetTexCoords(0.0f);
      joinsGeometry.push_back(LV(pivot, normals[3 * j], colorCoord, uv));
      joinsGeometry.push_back(LV(pivot, normals[3 * j + 1], colorCoord, uv));
      joinsGeometry.push_back(LV(pivot, normals[3 * j + 2], colorCoord, uv));
    }
  };

  // constuct segments
  vector<LineSegment> segments;
  segments.reserve(path.size() - 1);
  for (size_t i = 1; i < path.size(); ++i)
  {
    LineSegment segment;
    segment.m_points[StartPoint] = glsl::ToVec2(path[i - 1]);
    segment.m_points[EndPoint] = glsl::ToVec2(path[i]);
    CalculateTangentAndNormals(segment.m_points[StartPoint], segment.m_points[EndPoint], segment.m_tangent,
                               segment.m_leftBaseNormal, segment.m_rightBaseNormal);

    segment.m_leftNormals[StartPoint] = segment.m_leftNormals[EndPoint] = segment.m_leftBaseNormal;
    segment.m_rightNormals[StartPoint] = segment.m_rightNormals[EndPoint] = segment.m_rightBaseNormal;

    segments.push_back(segment);
  }

  // build geometry
  for (size_t i = 0; i < segments.size(); i++)
  {
    UpdateNormals(&segments[i], (i > 0) ? &segments[i - 1] : nullptr,
                 (i < segments.size() - 1) ? &segments[i + 1] : nullptr);

    // calculate number of steps to cover line segment
    float const initialGlobalLength = glsl::length(segments[i].m_points[EndPoint] - segments[i].m_points[StartPoint]);
    int steps = 1;
    float maskSize = initialGlobalLength;
    if (!texCoordGen.IsSolid())
    {
      float const leftWidth = glbHalfWidth * max(segments[i].m_leftWidthScalar[StartPoint].y,
                                                 segments[i].m_rightWidthScalar[StartPoint].y);
      float const rightWidth = glbHalfWidth * max(segments[i].m_leftWidthScalar[EndPoint].y,
                                                  segments[i].m_rightWidthScalar[EndPoint].y);
      float const effectiveLength = initialGlobalLength - leftWidth - rightWidth;
      if (effectiveLength > 0)
      {
        float const pixelLen = effectiveLength * m_params.m_baseGtoPScale;
        steps = static_cast<int>((pixelLen + texCoordGen.GetMaskLength() - 1) / texCoordGen.GetMaskLength());
        maskSize = effectiveLength / steps;
      }
    }

    // generate main geometry
    float currentSize = 0;
    glsl::vec3 currentStartPivot = glsl::vec3(segments[i].m_points[StartPoint], m_params.m_depth);
    for (int step = 0; step < steps; step++)
    {
      float const offsetFromStart = currentSize;
      currentSize += maskSize;

      glsl::vec2 const newPoint = segments[i].m_points[StartPoint] + segments[i].m_tangent * currentSize;
      glsl::vec3 const newPivot = glsl::vec3(newPoint, m_params.m_depth);

      ENormalType normalType1 = (step == 0) ? StartNormal : BaseNormal;
      ENormalType normalType2 = (step == steps - 1) ? EndNormal : BaseNormal;

      glsl::vec2 const leftNormal1 = GetNormal(segments[i], true /* isLeft */, normalType1);
      glsl::vec2 const rightNormal1 = GetNormal(segments[i], false /* isLeft */, normalType1);
      glsl::vec2 const leftNormal2 = GetNormal(segments[i], true /* isLeft */, normalType2);
      glsl::vec2 const rightNormal2 = GetNormal(segments[i], false /* isLeft */, normalType2);

      geometry.push_back(getLineVertex(segments[i], currentStartPivot, glsl::vec2(0, 0), offsetFromStart));
      geometry.push_back(getLineVertex(segments[i], currentStartPivot, leftNormal1, offsetFromStart));
      geometry.push_back(getLineVertex(segments[i], newPivot, glsl::vec2(0, 0), offsetFromStart));
      geometry.push_back(getLineVertex(segments[i], newPivot, leftNormal2, offsetFromStart));

      geometry.push_back(getLineVertex(segments[i], currentStartPivot, rightNormal1, offsetFromStart));
      geometry.push_back(getLineVertex(segments[i], currentStartPivot, glsl::vec2(0, 0), offsetFromStart));
      geometry.push_back(getLineVertex(segments[i], newPivot, rightNormal2, offsetFromStart));
      geometry.push_back(getLineVertex(segments[i], newPivot, glsl::vec2(0, 0), offsetFromStart));

      currentStartPivot = newPivot;
    }

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
      GenerateJoinNormals(m_params.m_join, n1, n2, halfWidth, segments[i].m_hasLeftJoin[EndPoint],
                          widthScalar, normals);

      generateTriangles(glsl::vec3(segments[i].m_points[EndPoint], m_params.m_depth), normals);
    }

    // generate caps
    if (i == 0)
    {
      vector<glsl::vec2> normals;
      normals.reserve(24);
      GenerateCapNormals(m_params.m_cap, segments[i].m_leftNormals[StartPoint],
                         segments[i].m_rightNormals[StartPoint], -segments[i].m_tangent,
                         halfWidth, true /* isStart */, normals);

      generateTriangles(glsl::vec3(segments[i].m_points[StartPoint], m_params.m_depth), normals);
    }

    if (i == segments.size() - 1)
    {
      vector<glsl::vec2> normals;
      normals.reserve(24);
      GenerateCapNormals(m_params.m_cap, segments[i].m_leftNormals[EndPoint],
                         segments[i].m_rightNormals[EndPoint], segments[i].m_tangent,
                         halfWidth, false /* isStart */, normals);

      generateTriangles(glsl::vec3(segments[i].m_points[EndPoint], m_params.m_depth), normals);
    }
  }

  dp::GLState state(gpu::LINE_PROGRAM, dp::GLState::GeometryLayer);
  state.SetColorTexture(colorRegion.GetTexture());
  state.SetMaskTexture(maskRegion.GetTexture());

  dp::AttributeProvider provider(1, geometry.size());
  provider.InitStream(0, gpu::LineVertex::GetBindingInfo(), make_ref(geometry.data()));
  batcher->InsertListOfStrip(state, make_ref(&provider), 4);

  dp::AttributeProvider joinsProvider(1, joinsGeometry.size());
  joinsProvider.InitStream(0, gpu::LineVertex::GetBindingInfo(), make_ref(joinsGeometry.data()));
  batcher->InsertTriangleList(state, make_ref(&joinsProvider));
}

} // namespace df

