#pragma once

#include "drape/drape_global.hpp"
#include "drape/glsl_types.hpp"

#include "std/vector.hpp"

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

  LineSegment()
  {
    m_leftWidthScalar[StartPoint] = m_leftWidthScalar[EndPoint] = glsl::vec2(1.0f, 0.0f);
    m_rightWidthScalar[StartPoint] = m_rightWidthScalar[EndPoint] = glsl::vec2(1.0f, 0.0f);
    m_hasLeftJoin[StartPoint] = m_hasLeftJoin[EndPoint] = true;
    m_generateJoin = true;
  }
};

void ConstructLineSegments(vector<m2::PointD> const & path, vector<LineSegment> & segments);

void UpdateNormals(LineSegment * segment, LineSegment * prevSegment, LineSegment * nextSegment);

void GenerateJoinNormals(dp::LineJoin joinType, glsl::vec2 const & normal1, glsl::vec2 const & normal2,
                         float halfWidth, bool isLeft, float widthScalar, vector<glsl::vec2> & normals);

void GenerateCapNormals(dp::LineCap capType, glsl::vec2 const & normal1, glsl::vec2 const & normal2,
                        glsl::vec2 const & direction, float halfWidth, bool isStart, vector<glsl::vec2> & normals);

glsl::vec2 GetNormal(LineSegment const & segment, bool isLeft, ENormalType normalType);

float GetProjectionLength(glsl::vec2 const & newPoint, glsl::vec2 const & startPoint, glsl::vec2 const & endPoint);

} // namespace df

