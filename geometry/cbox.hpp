#pragma once

#include "geometry/point2d.hpp"

#include <vector>

namespace m2
{
class CBox
{
public:
  CBox(std::vector<m2::PointD> const & points);

  std::vector<m2::PointD> Points() const { return m_points; }

  bool IsInside(m2::PointD const & p) const;
  bool IsInside(double x, double y) const { return IsInside(m2::PointD(x, y)); }

private:
  std::vector<m2::PointD> m_points;
};
}  // namespace m2
