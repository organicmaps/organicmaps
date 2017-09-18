#pragma once

#include "geometry/bbox.hpp"
#include "geometry/point2d.hpp"

namespace m2
{
class DBox
{
public:
  void Add(m2::PointD const & p) { return Add(p.x, p.y); }
  void Add(double x, double y) { return m_box.Add(x + y, x - y); }

  bool IsInside(m2::PointD const & p) const { return IsInside(p.x, p.y); }
  bool IsInside(double x, double y) const { return m_box.IsInside(x + y, x - y); }

private:
  BBox m_box;
};
}  // namespace m2
