#pragma once

#include "geometry/point2d.hpp"

#include <vector>

namespace m2
{
// When the size of the convex hull over a set of |points| is less
// than 4, stores convex hull explicitly, otherwise stores smallest
// rectangle containing the hull. Note that at least one side of the
// rectangle should contain a facet of the hull, and in general sides
// of the rectangle may not be parallel to the axes. In any case,
// CalipersBox stores points in counter-clockwise order.
class CalipersBox
{
public:
  CalipersBox(std::vector<PointD> const & points);

  std::vector<PointD> const & Points() const { return m_points; }

  bool HasPoint(PointD const & p) const;
  bool HasPoint(double x, double y) const { return HasPoint(PointD(x, y)); }

private:
  std::vector<PointD> m_points;
};
}  // namespace m2
