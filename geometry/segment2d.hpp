#pragma once

#include "geometry/point2d.hpp"

#include <string>

namespace m2
{
struct Segment2D
{
  Segment2D() = default;
  Segment2D(m2::PointD const & u, m2::PointD const & v) : m_u(u), m_v(v) {}

  m2::PointD const Dir() const { return m_v - m_u; }

  m2::PointD m_u;
  m2::PointD m_v;
};

std::string DebugPrint(Segment2D const & s);

bool IsPointOnSegmentEps(m2::PointD const & pt, m2::PointD const & p1, m2::PointD const & p2,
                         double eps);
bool IsPointOnSegment(m2::PointD const & pt, m2::PointD const & p1, m2::PointD const & p2);
}  // namespace m2
