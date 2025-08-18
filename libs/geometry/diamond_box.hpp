#pragma once

#include "geometry/bounding_box.hpp"
#include "geometry/point2d.hpp"

#include "base/visitor.hpp"

#include <vector>

namespace m2
{
// Bounding box for a set of points on the plane, rotated by 45
// degrees.
class DiamondBox
{
public:
  DiamondBox() = default;
  explicit DiamondBox(std::vector<PointD> const & points);

  void Add(PointD const & p) { return Add(p.x, p.y); }
  void Add(double x, double y) { return m_box.Add(x + y, x - y); }

  bool HasPoint(PointD const & p) const { return HasPoint(p.x, p.y); }
  bool HasPoint(double x, double y) const { return m_box.HasPoint(x + y, x - y); }

  bool HasPoint(PointD const & p, double eps) const { return HasPoint(p.x, p.y, eps); }

  bool HasPoint(double x, double y, double eps) const { return m_box.HasPoint(x + y, x - y, 2 * eps); }

  std::vector<m2::PointD> Points() const
  {
    auto points = m_box.Points();
    for (auto & p : points)
      p = ToOrig(p);
    return points;
  }

  bool operator==(DiamondBox const & rhs) const { return m_box == rhs.m_box; }

  DECLARE_VISITOR(visitor(m_box, "box"))
  DECLARE_DEBUG_PRINT(DiamondBox)

private:
  static m2::PointD ToOrig(m2::PointD const & p) { return m2::PointD(0.5 * (p.x + p.y), 0.5 * (p.x - p.y)); }

  BoundingBox m_box;
};
}  // namespace m2
