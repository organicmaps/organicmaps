#include "drape_frontend/line_shape_helper.hpp"

#include "drape/glsl_func.hpp"

#include "base/assert.hpp"

namespace df
{

namespace
{

void UpdateNormalBetweenSegments(LineSegment * segment1, LineSegment * segment2)
{
  ASSERT(segment1 != nullptr && segment2 != nullptr, ());

  float const dotProduct = glsl::dot(segment1->m_leftNormals[EndPoint],
                                     segment2->m_leftNormals[StartPoint]);
  float const absDotProduct = fabs(dotProduct);
  float const kEps = 1e-5;

  if (fabs(absDotProduct - 1.0f) < kEps)
  {
    // change nothing
    return;
  }

  float const kMaxScalar = 5;
  float const crossProduct = glsl::cross(glsl::vec3(segment1->m_tangent, 0),
                                         glsl::vec3(segment2->m_tangent, 0)).z;
  if (crossProduct < 0)
  {
    segment1->m_hasLeftJoin[EndPoint] = true;
    segment2->m_hasLeftJoin[StartPoint] = true;

    // change right-side normals
    glsl::vec2 averageNormal = glsl::normalize(segment1->m_rightNormals[EndPoint] +
                                               segment2->m_rightNormals[StartPoint]);
    float const cosAngle = glsl::dot(segment1->m_tangent, averageNormal);
    float const widthScalar = 1.0f / sqrt(1.0f - cosAngle * cosAngle);
    if (widthScalar < kMaxScalar)
    {
      segment1->m_rightNormals[EndPoint] = averageNormal;
      segment2->m_rightNormals[StartPoint] = averageNormal;
      segment1->m_rightWidthScalar[EndPoint].x = widthScalar;
      segment1->m_rightWidthScalar[EndPoint].y = widthScalar * cosAngle;
      segment2->m_rightWidthScalar[StartPoint] = segment1->m_rightWidthScalar[EndPoint];
    }
    else
    {
      segment1->m_generateJoin = false;
    }
  }
  else
  {
    segment1->m_hasLeftJoin[EndPoint] = false;
    segment2->m_hasLeftJoin[StartPoint] = false;

    // change left-side normals
    glsl::vec2 averageNormal = glsl::normalize(segment1->m_leftNormals[EndPoint] +
                                               segment2->m_leftNormals[StartPoint]);
    float const cosAngle = glsl::dot(segment1->m_tangent, averageNormal);
    float const widthScalar = 1.0f / sqrt(1.0f - cosAngle * cosAngle);
    if (widthScalar < kMaxScalar)
    {
      segment1->m_leftNormals[EndPoint] = averageNormal;
      segment2->m_leftNormals[StartPoint] = averageNormal;
      segment1->m_leftWidthScalar[EndPoint].x = widthScalar;
      segment1->m_leftWidthScalar[EndPoint].y = widthScalar * cosAngle;
      segment2->m_leftWidthScalar[StartPoint] = segment1->m_leftWidthScalar[EndPoint];
    }
    else
    {
      segment1->m_generateJoin = false;
    }
  }
}

void CalculateTangentAndNormals(glsl::vec2 const & pt0, glsl::vec2 const & pt1,
                                glsl::vec2 & tangent, glsl::vec2 & leftNormal,
                                glsl::vec2 & rightNormal)
{
  tangent = glsl::normalize(pt1 - pt0);
  leftNormal = glsl::vec2(-tangent.y, tangent.x);
  rightNormal = -leftNormal;
}

}

void ConstructLineSegments(vector<m2::PointD> const & path, vector<LineSegment> & segments)
{
  ASSERT(path.size() > 1, ());

  float const eps = 1e-5;

  m2::PointD prevPoint = path[0];
  for (size_t i = 1; i < path.size(); ++i)
  {
    // filter the same points
    if (prevPoint.EqualDxDy(path[i], eps))
      continue;

    LineSegment segment;

    segment.m_points[StartPoint] = glsl::ToVec2(prevPoint);
    segment.m_points[EndPoint] = glsl::ToVec2(path[i]);
    CalculateTangentAndNormals(segment.m_points[StartPoint], segment.m_points[EndPoint], segment.m_tangent,
                               segment.m_leftBaseNormal, segment.m_rightBaseNormal);

    segment.m_leftNormals[StartPoint] = segment.m_leftNormals[EndPoint] = segment.m_leftBaseNormal;
    segment.m_rightNormals[StartPoint] = segment.m_rightNormals[EndPoint] = segment.m_rightBaseNormal;

    prevPoint = path[i];

    segments.push_back(segment);
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
  float const eps = 1e-5;
  if (fabs(glsl::dot(normal1, normal2) - 1.0f) < eps)
    return;

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

glsl::vec2 GetNormal(LineSegment const & segment, bool isLeft, ENormalType normalType)
{
  if (normalType == BaseNormal)
    return isLeft ? segment.m_leftBaseNormal : segment.m_rightBaseNormal;

  int const index = (normalType == StartNormal) ? StartPoint : EndPoint;
  return isLeft ? segment.m_leftWidthScalar[index].x * segment.m_leftNormals[index]:
                  segment.m_rightWidthScalar[index].x * segment.m_rightNormals[index];
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

} // namespace df

