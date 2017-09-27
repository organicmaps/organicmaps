#pragma once

#include "geometry/point2d.hpp"

#include "base/visitor.hpp"

#include <string>
#include <vector>

namespace m2
{
// When the size of the convex hull over a set of |points| is less
// than 3, stores convex hull explicitly, otherwise stores smallest
// rectangle containing the hull. Note that at least one side of the
// rectangle should contain a facet of the hull, and in general sides
// of the rectangle may not be parallel to the axes. In any case,
// CalipersBox stores points in counter-clockwise order.
class CalipersBox
{
public:
  CalipersBox() = default;
  CalipersBox(std::vector<PointD> const & points);

  std::vector<PointD> const & Points() const { return m_points; }

  bool HasPoint(PointD const & p) const;
  bool HasPoint(double x, double y) const { return HasPoint(PointD(x, y)); }

  bool operator==(CalipersBox const & rhs) const { return m_points == rhs.m_points; }

  DECLARE_VISITOR(visitor(m_points))

private:
  std::vector<PointD> m_points;
};

std::string DebugPrint(CalipersBox const & cbox);
}  // namespace m2
