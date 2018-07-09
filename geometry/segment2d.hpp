#pragma once

#include "geometry/point2d.hpp"

#include <string>

namespace m2
{
struct Segment2D
{
  Segment2D() = default;
  Segment2D(PointD const & u, PointD const & v) : m_u(u), m_v(v) {}

  PointD const Dir() const { return m_v - m_u; }

  PointD m_u;
  PointD m_v;
};

std::string DebugPrint(Segment2D const & s);

bool IsPointOnSegmentEps(PointD const & pt, PointD const & p1, PointD const & p2, double eps);
bool IsPointOnSegment(PointD const & pt, PointD const & p1, PointD const & p2);
}  // namespace m2
