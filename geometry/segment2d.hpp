#pragma once

#include "geometry/point2d.hpp"

#include <string>

namespace m2
{
struct IntersectionResult
{
  enum class Type
  {
    Zero,
    One,
    Infinity
  };

  explicit IntersectionResult(Type type) : m_type(type) { ASSERT_NOT_EQUAL(m_type, Type::One, ()); }
  explicit IntersectionResult(PointD const & point) : m_point(point), m_type(Type::One) {}

  PointD m_point;
  Type m_type;
};

struct Segment2D
{
  Segment2D() = default;
  Segment2D(PointD const & u, PointD const & v) : m_u(u), m_v(v) {}

  PointD const Dir() const { return m_v - m_u; }

  PointD m_u;
  PointD m_v;
};

bool IsPointOnSegmentEps(PointD const & pt, PointD const & p1, PointD const & p2, double eps);
bool IsPointOnSegment(PointD const & pt, PointD const & p1, PointD const & p2);

/// \returns true if segments (p1, p2) and (p3, p4) are intersected and false otherwise.
bool SegmentsIntersect(PointD const & p1, PointD const & p2, PointD const & p3, PointD const & p4);

/// \breif Intersects two segments and finds an intersection point if any.
/// \note |eps| applies for collinearity and for PointD errors.
IntersectionResult Intersect(Segment2D const & seg1, Segment2D const & seg2, double eps);

std::string DebugPrint(Segment2D const & s);
std::string DebugPrint(IntersectionResult::Type type);
std::string DebugPrint(IntersectionResult const & result);
}  // namespace m2
