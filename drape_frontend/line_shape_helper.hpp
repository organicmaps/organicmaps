#pragma once

#include "drape/drape_global.hpp"
#include "drape/glsl_types.hpp"

#include "geometry/rect2d.hpp"

#include <vector>

namespace df
{
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
  bool m_generateJoin;
  glsl::vec4 m_color;

  LineSegment(glsl::vec2 const & p1, glsl::vec2 const & p2)
  {
    m_points[StartPoint] = p1;
    m_points[EndPoint] = p2;
    m_leftWidthScalar[StartPoint] = m_leftWidthScalar[EndPoint] = glsl::vec2(1.0f, 0.0f);
    m_rightWidthScalar[StartPoint] = m_rightWidthScalar[EndPoint] = glsl::vec2(1.0f, 0.0f);
    m_hasLeftJoin[StartPoint] = m_hasLeftJoin[EndPoint] = true;
    m_color = glsl::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    m_generateJoin = true;
  }
};

void CalculateTangentAndNormals(glsl::vec2 const & pt0, glsl::vec2 const & pt1, glsl::vec2 & tangent,
                                glsl::vec2 & leftNormal, glsl::vec2 & rightNormal);

void ConstructLineSegments(std::vector<m2::PointD> const & path, std::vector<glsl::vec4> const & segmentsColors,
                           std::vector<LineSegment> & segments);

void UpdateNormals(LineSegment * segment, LineSegment * prevSegment, LineSegment * nextSegment);

std::vector<glsl::vec2> GenerateJoinNormals(dp::LineJoin joinType, glsl::vec2 const & normal1,
                                            glsl::vec2 const & normal2, float halfWidth, bool isLeft, float widthScalar,
                                            std::vector<glsl::vec2> * uv = nullptr);

std::vector<glsl::vec2> GenerateCapNormals(dp::LineCap capType, glsl::vec2 const & normal1, glsl::vec2 const & normal2,
                                           glsl::vec2 const & direction, float halfWidth, bool isStart,
                                           int segmentsCount = 8);

glsl::vec2 GetNormal(LineSegment const & segment, bool isLeft, ENormalType normalType);

float GetProjectionLength(glsl::vec2 const & newPoint, glsl::vec2 const & startPoint, glsl::vec2 const & endPoint);
}  // namespace df
