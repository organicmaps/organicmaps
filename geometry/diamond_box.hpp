#pragma once

#include "geometry/bounding_box.hpp"
#include "geometry/point2d.hpp"

namespace m2
{
// Bounding box for a set of points on the plane, rotated by 45
// degrees.
class DiamondBox
{
public:
  void Add(PointD const & p) { return Add(p.x, p.y); }
  void Add(double x, double y) { return m_box.Add(x + y, x - y); }

  bool HasPoint(PointD const & p) const { return HasPoint(p.x, p.y); }
  bool HasPoint(double x, double y) const { return m_box.HasPoint(x + y, x - y); }

private:
  BoundingBox m_box;
};
}  // namespace m2
