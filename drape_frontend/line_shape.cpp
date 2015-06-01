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
  float const SEGMENT = 0.0f;
  float const CAP = 1.0f;
  float const LEFT_WIDTH = 1.0f;
  float const RIGHT_WIDTH = -1.0f;

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
    float m_leftWidthScalar[PointsCount];
    float m_rightWidthScalar[PointsCount];
    bool m_hasLeftJoin[PointsCount];

    LineSegment()
    {
      m_leftWidthScalar[StartPoint] = m_leftWidthScalar[EndPoint] = 1.0f;
      m_rightWidthScalar[StartPoint] = m_rightWidthScalar[EndPoint] = 1.0f;
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
      segment1->m_rightWidthScalar[EndPoint] = 1.0f / sqrt(1.0f - cosAngle * cosAngle);
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
      segment1->m_leftWidthScalar[EndPoint] = 1.0f / sqrt(1.0f - cosAngle * cosAngle);
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

  /*uint32_t BuildRect(vector<Button::ButtonVertex> & vertices,
                     glsl::vec2 const & v1, glsl::vec2 const & v2,
                     glsl::vec2 const & v3, glsl::vec2 const & v4)

  {
    glsl::vec3 const position(0.0f, 0.0f, 0.0f);

    vertices.push_back(Button::ButtonVertex(position, v1));
    vertices.push_back(Button::ButtonVertex(position, v2));
    vertices.push_back(Button::ButtonVertex(position, v3));

    vertices.push_back(Button::ButtonVertex(position, v3));
    vertices.push_back(Button::ButtonVertex(position, v2));
    vertices.push_back(Button::ButtonVertex(position, v4));

    return dp::Batcher::IndexPerQuad;
  }*/

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
  typedef gpu::LineVertex LV;
  buffer_vector<gpu::LineVertex, 128> geometry;
  buffer_vector<gpu::LineVertex, 128> joinsGeometry;
  vector<m2::PointD> const & path = m_spline->GetPath();
  ASSERT(path.size() > 1, ());

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
  bool generateCap = m_params.m_cap != dp::ButtCap;

  auto const calcTangentAndNormals = [](glsl::vec2 const & pt0, glsl::vec2 const & pt1,
                                        glsl::vec2 & tangent, glsl::vec2 & leftNormal,
                                        glsl::vec2 & rightNormal)
  {
    tangent = glsl::normalize(pt1 - pt0);
    leftNormal = glsl::vec2(-tangent.y, tangent.x);
    rightNormal = -leftNormal;
  };

  auto const getNormal = [&halfWidth](LineSegment const & segment, bool isLeft, ENormalType normalType)
  {
    if (normalType == BaseNormal)
      return halfWidth * (isLeft ? segment.m_leftBaseNormal : segment.m_rightBaseNormal);

    int const index = (normalType == StartNormal) ? StartPoint : EndPoint;
    return halfWidth * (isLeft ? segment.m_leftWidthScalar[index] * segment.m_leftNormals[index] :
                                    segment.m_rightWidthScalar[index] * segment.m_rightNormals[index]);
  };

  float capType = m_params.m_cap == dp::RoundCap ? CAP : SEGMENT;

  glsl::vec2 leftSegment(SEGMENT, LEFT_WIDTH);
  glsl::vec2 rightSegment(SEGMENT, RIGHT_WIDTH);

  /*if (generateCap)
  {
    glsl::vec2 startPoint = glsl::ToVec2(path[0]);
    glsl::vec2 endPoint = glsl::ToVec2(path[1]);
    glsl::vec2 tangent, leftNormal, rightNormal;
    calcTangentAndNormals(startPoint, endPoint, tangent, leftNormal, rightNormal);
    tangent = -halfWidth * tangent;

    glsl::vec3 pivot = glsl::vec3(startPoint, m_params.m_depth);
    glsl::vec2 leftCap(capType, LEFT_WIDTH);
    glsl::vec2 rightCap(capType, RIGHT_WIDTH);
    glsl::vec2 const uvStart = texCoordGen.GetTexCoordsByDistance(0.0);
    glsl::vec2 const uvEnd = texCoordGen.GetTexCoordsByDistance(glbHalfWidth);

    geometry.push_back(LV(pivot, leftNormal + tangent, colorCoord, uvStart, leftCap));
    geometry.push_back(LV(pivot, rightNormal + tangent, colorCoord, uvStart, rightCap));
    geometry.push_back(LV(pivot, leftNormal, colorCoord, uvEnd, leftSegment));
    geometry.push_back(LV(pivot, rightNormal, colorCoord, uvEnd, rightSegment));
  }*/

  // constuct segments
  vector<LineSegment> segments;
  segments.reserve(path.size() - 1);
  for (size_t i = 1; i < path.size(); ++i)
  {
    LineSegment segment;
    segment.m_points[StartPoint] = glsl::ToVec2(path[i - 1]);
    segment.m_points[EndPoint] = glsl::ToVec2(path[i]);
    calcTangentAndNormals(segment.m_points[StartPoint], segment.m_points[EndPoint], segment.m_tangent,
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

    float const initialGlobalLength = glsl::length(segments[i].m_points[EndPoint] - segments[i].m_points[StartPoint]);
    float const pixelLen = initialGlobalLength * m_params.m_baseGtoPScale;
    int steps = 1;
    float maskSize = initialGlobalLength;
    if (!texCoordGen.IsSolid())
    {
      maskSize = texCoordGen.GetMaskLength() / m_params.m_baseGtoPScale;
      steps = static_cast<int>((pixelLen + texCoordGen.GetMaskLength() - 1) / texCoordGen.GetMaskLength());
    }

    // generate main geometry
    float currentSize = 0;
    glsl::vec3 currentStartPivot = glsl::vec3(segments[i].m_points[StartPoint], m_params.m_depth);
    for (int step = 0; step < steps; step++)
    {
      currentSize += maskSize;

      float endOffset = 1.0f;
      if (currentSize >= initialGlobalLength)
      {
        endOffset = (initialGlobalLength - currentSize + maskSize) / maskSize;
        currentSize = initialGlobalLength;
      }

      glsl::vec2 const newPoint = segments[i].m_points[StartPoint] + segments[i].m_tangent * currentSize;
      glsl::vec3 const newPivot = glsl::vec3(newPoint, m_params.m_depth);
      glsl::vec2 const uvStart = texCoordGen.GetTexCoords(0.0f);
      glsl::vec2 const uvEnd = texCoordGen.GetTexCoords(endOffset);

      ENormalType normalType1 = (step == 0) ? StartNormal : BaseNormal;
      ENormalType normalType2 = (step == steps - 1) ? EndNormal : BaseNormal;

      glsl::vec2 const leftNormal1 = getNormal(segments[i], true /* isLeft */, normalType1);
      glsl::vec2 const rightNormal1 = getNormal(segments[i], false /* isLeft */, normalType1);
      glsl::vec2 const leftNormal2 = getNormal(segments[i], true /* isLeft */, normalType2);
      glsl::vec2 const rightNormal2 = getNormal(segments[i], false /* isLeft */, normalType2);

      geometry.push_back(LV(currentStartPivot, glsl::vec2(0, 0), colorCoord, uvStart, rightSegment));
      geometry.push_back(LV(currentStartPivot, leftNormal1, colorCoord, uvStart, leftSegment));
      geometry.push_back(LV(newPivot, glsl::vec2(0, 0), colorCoord, uvEnd, rightSegment));
      geometry.push_back(LV(newPivot, leftNormal2, colorCoord, uvEnd, leftSegment));

      geometry.push_back(LV(currentStartPivot, rightNormal1, colorCoord, uvStart, rightSegment));
      geometry.push_back(LV(currentStartPivot, glsl::vec2(0, 0), colorCoord, uvStart, leftSegment));
      geometry.push_back(LV(newPivot, rightNormal2, colorCoord, uvEnd, rightSegment));
      geometry.push_back(LV(newPivot, glsl::vec2(0, 0), colorCoord, uvEnd, leftSegment));

      currentStartPivot = newPivot;
    }

    // generate joins
    if (i < segments.size() - 1)
    {
      glsl::vec2 n1 = segments[i].m_hasLeftJoin[EndPoint] ? segments[i].m_leftNormals[EndPoint] :
                                                            segments[i].m_rightNormals[EndPoint];
      glsl::vec2 n2 = segments[i + 1].m_hasLeftJoin[StartPoint] ? segments[i + 1].m_leftNormals[StartPoint] :
                                                                  segments[i + 1].m_rightNormals[StartPoint];

      float widthScalar = segments[i].m_hasLeftJoin[EndPoint] ? segments[i].m_rightWidthScalar[EndPoint] :
                                                                segments[i].m_leftWidthScalar[EndPoint];

      vector<glsl::vec2> normals;
      normals.reserve(24);
      GenerateJoinNormals(m_params.m_join, n1, n2, halfWidth, segments[i].m_hasLeftJoin[EndPoint],
                          widthScalar, normals);

      glsl::vec3 const joinPivot = glsl::vec3(segments[i].m_points[EndPoint], m_params.m_depth);

      size_t const trianglesCount = normals.size() / 3;
      for (int j = 0; j < trianglesCount; j++)
      {
        glsl::vec2 const uv = texCoordGen.GetTexCoords(0.0f); //TODO

        joinsGeometry.push_back(LV(joinPivot, normals[3 * j], colorCoord, uv, leftSegment));
        joinsGeometry.push_back(LV(joinPivot, normals[3 * j + 1], colorCoord, uv, rightSegment));
        joinsGeometry.push_back(LV(joinPivot, normals[3 * j + 2], colorCoord, uv, leftSegment));
      }
    }

    // generate caps
    if (i == 0)
    {
      vector<glsl::vec2> normals;
      normals.reserve(24);
      GenerateCapNormals(m_params.m_cap, segments[i].m_leftNormals[StartPoint],
                         segments[i].m_rightNormals[StartPoint], -segments[i].m_tangent,
                         halfWidth, true /* isStart */, normals);

      glsl::vec3 const joinPivot = glsl::vec3(segments[i].m_points[StartPoint], m_params.m_depth);

      size_t const trianglesCount = normals.size() / 3;
      for (int j = 0; j < trianglesCount; j++)
      {
        glsl::vec2 const uv = texCoordGen.GetTexCoords(0.0f); //TODO

        joinsGeometry.push_back(LV(joinPivot, normals[3 * j], colorCoord, uv, leftSegment));
        joinsGeometry.push_back(LV(joinPivot, normals[3 * j + 1], colorCoord, uv, rightSegment));
        joinsGeometry.push_back(LV(joinPivot, normals[3 * j + 2], colorCoord, uv, leftSegment));
      }
    }

    if (i == segments.size() - 1)
    {
      vector<glsl::vec2> normals;
      normals.reserve(24);
      GenerateCapNormals(m_params.m_cap, segments[i].m_leftNormals[EndPoint],
                         segments[i].m_rightNormals[EndPoint], segments[i].m_tangent,
                         halfWidth, false /* isStart */, normals);

      glsl::vec3 const joinPivot = glsl::vec3(segments[i].m_points[EndPoint], m_params.m_depth);

      size_t const trianglesCount = normals.size() / 3;
      for (int j = 0; j < trianglesCount; j++)
      {
        glsl::vec2 const uv = texCoordGen.GetTexCoords(0.0f); //TODO

        joinsGeometry.push_back(LV(joinPivot, normals[3 * j], colorCoord, uv, leftSegment));
        joinsGeometry.push_back(LV(joinPivot, normals[3 * j + 1], colorCoord, uv, rightSegment));
        joinsGeometry.push_back(LV(joinPivot, normals[3 * j + 2], colorCoord, uv, leftSegment));
      }
    }
  }

  //glsl::vec2 prevPoint;
  //glsl::vec2 prevLeftNormal;
  //glsl::vec2 prevRightNormal;

  /*for (size_t i = 1; i < path.size(); ++i)
  {
    glsl::vec2 startPoint = glsl::ToVec2(path[i - 1]);
    glsl::vec2 endPoint = glsl::ToVec2(path[i]);
    glsl::vec2 tangent, leftNormal, rightNormal;
    calcTangentAndNormals(startPoint, endPoint, tangent, leftNormal, rightNormal);

    glsl::vec3 startPivot = glsl::vec3(startPoint, m_params.m_depth);

    // Create join beetween current segment and previous
    if (i > 1)
    {
      glsl::vec2 zeroNormal(0.0, 0.0);
      glsl::vec2 prevForming, nextForming;
      if (glsl::dot(prevLeftNormal, tangent) < 0)
      {
        prevForming = prevLeftNormal;
        nextForming = leftNormal;
      }
      else
      {
        prevForming = prevRightNormal;
        nextForming = rightNormal;
      }

      float const distance = glsl::length(nextForming - prevForming) / m_params.m_baseGtoPScale;
      float const endUv = min(1.0f, texCoordGen.GetUvOffsetByDistance(distance));
      glsl::vec2 const uvStart = texCoordGen.GetTexCoords(0.0f);
      glsl::vec2 const uvEnd = texCoordGen.GetTexCoords(endUv);

      if (m_params.m_join == dp::BevelJoin)
      {
        geometry.push_back(LV(startPivot, prevForming, colorCoord, uvStart, leftSegment));
        geometry.push_back(LV(startPivot, zeroNormal, colorCoord, uvStart, leftSegment));
        geometry.push_back(LV(startPivot, nextForming, colorCoord, uvEnd, leftSegment));
        geometry.push_back(LV(startPivot, nextForming, colorCoord, uvEnd, leftSegment));
      }
      else
      {
        glsl::vec2 middleForming = glsl::normalize(prevForming + nextForming);
        glsl::vec2 zeroDxDy(0.0, 0.0);

        float const middleUv = 0.5f * endUv;
        glsl::vec2 const uvMiddle = texCoordGen.GetTexCoords(middleUv);

        if (m_params.m_join == dp::MiterJoin)
        {
          float const b = 0.5f * glsl::length(prevForming - nextForming);
          float const a = glsl::length(prevForming);
          middleForming *= static_cast<float>(sqrt(a * a + b * b));

          geometry.push_back(LV(startPivot, prevForming, colorCoord, uvStart, zeroDxDy));
          geometry.push_back(LV(startPivot, zeroNormal, colorCoord, uvStart, zeroDxDy));
          geometry.push_back(LV(startPivot, middleForming, colorCoord, uvMiddle, zeroDxDy));
          geometry.push_back(LV(startPivot, nextForming, colorCoord, uvEnd, zeroDxDy));
        }
        else
        {
          middleForming *= glsl::length(prevForming);

          glsl::vec2 dxdyLeft(0.0, -1.0);
          glsl::vec2 dxdyRight(0.0, -1.0);
          glsl::vec2 dxdyMiddle(1.0, 1.0);
          geometry.push_back(LV(startPivot, zeroNormal, colorCoord, uvStart, zeroDxDy));
          geometry.push_back(LV(startPivot, prevForming, colorCoord, uvStart, dxdyLeft));
          geometry.push_back(LV(startPivot, nextForming, colorCoord, uvEnd, dxdyRight));
          geometry.push_back(LV(startPivot, middleForming, colorCoord, uvMiddle, dxdyMiddle));
        }
      }
    }

    float initialGlobalLength = glsl::length(endPoint - startPoint);
    float pixelLen = initialGlobalLength * m_params.m_baseGtoPScale;
    int steps = 1;
    float maskSize = initialGlobalLength;
    if (!texCoordGen.IsSolid())
    {
      maskSize = texCoordGen.GetMaskLength() / m_params.m_baseGtoPScale;
      steps = static_cast<int>((pixelLen + texCoordGen.GetMaskLength() - 1) / texCoordGen.GetMaskLength());
    }

    float currentSize = 0;
    glsl::vec3 currentStartPivot = startPivot;
    for (int i = 0; i < steps; i++)
    {
      currentSize += maskSize;

      float endOffset = 1.0f;
      if (currentSize >= initialGlobalLength)
      {
        endOffset = (initialGlobalLength - currentSize + maskSize) / maskSize;
        currentSize = initialGlobalLength;
      }

      glsl::vec2 const newPoint = startPoint + tangent * currentSize;
      glsl::vec3 const newPivot = glsl::vec3(newPoint, m_params.m_depth);
      glsl::vec2 const uvStart = texCoordGen.GetTexCoords(0.0f);
      glsl::vec2 const uvEnd = texCoordGen.GetTexCoords(endOffset);

      geometry.push_back(LV(currentStartPivot, leftNormal, colorCoord, uvStart, leftSegment));
      geometry.push_back(LV(currentStartPivot, rightNormal, colorCoord, uvStart, rightSegment));
      geometry.push_back(LV(newPivot, leftNormal, colorCoord, uvEnd, leftSegment));
      geometry.push_back(LV(newPivot, rightNormal, colorCoord, uvEnd, rightSegment));

      currentStartPivot = newPivot;
    }

    prevPoint = currentStartPivot.xy();
    prevLeftNormal = leftNormal;
    prevRightNormal = rightNormal;
  }*/

  /*if (generateCap)
  {
    size_t lastPointIndex = path.size() - 1;
    glsl::vec2 startPoint = glsl::ToVec2(path[lastPointIndex - 1]);
    glsl::vec2 endPoint = glsl::ToVec2(path[lastPointIndex]);
    glsl::vec2 tangent, leftNormal, rightNormal;
    calcTangentAndNormals(startPoint, endPoint, tangent, leftNormal, rightNormal);
    tangent = halfWidth * tangent;

    glsl::vec3 pivot = glsl::vec3(endPoint, m_params.m_depth);
    glsl::vec2 leftCap(capType, LEFT_WIDTH);
    glsl::vec2 rightCap(capType, RIGHT_WIDTH);
    glsl::vec2 const uvStart = texCoordGen.GetTexCoordsByDistance(0.0);
    glsl::vec2 const uvEnd = texCoordGen.GetTexCoordsByDistance(glbHalfWidth);

    geometry.push_back(LV(pivot, leftNormal, colorCoord, uvStart, leftSegment));
    geometry.push_back(LV(pivot, rightNormal, colorCoord, uvStart, rightSegment));
    geometry.push_back(LV(pivot, leftNormal + tangent, colorCoord, uvEnd, leftCap));
    geometry.push_back(LV(pivot, rightNormal + tangent, colorCoord, uvEnd, rightCap));
  }*/

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

