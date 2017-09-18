#pragma once

#include "geometry/point2d.hpp"
#include "geometry/segment2d.hpp"

#include "base/assert.hpp"

#include <string>

namespace m2
{
struct Line2D
{
  Line2D() = default;

  explicit Line2D(Segment2D const & segment) : m_point(segment.m_u), m_direction(segment.Dir()) {}

  Line2D(m2::PointD const & point, m2::PointD const & direction)
    : m_point(point), m_direction(direction)
  {
  }

  m2::PointD m_point;
  m2::PointD m_direction;
};

std::string DebugPrint(Line2D const & line);

struct LineIntersector
{
  struct Result
  {
    enum class Type
    {
      Zero,
      Single,
      Infinite
    };

    explicit Result(Type type) : m_type(type) { ASSERT_NOT_EQUAL(m_type, Type::Single, ()); }
    explicit Result(m2::PointD const & point) : m_point(point), m_type(Type::Single) {}

    m2::PointD m_point;
    Type m_type;
  };

  static Result Intersect(Line2D const & lhs, Line2D const & rhs, double eps);
};

std::string DebugPrint(LineIntersector::Result::Type type);
std::string DebugPrint(LineIntersector::Result const & result);
}  // namespace m2
