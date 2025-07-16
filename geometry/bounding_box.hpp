#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/visitor.hpp"

#include <limits>
#include <vector>

namespace m2
{
class BoundingBox
{
public:
  BoundingBox() = default;
  BoundingBox(std::vector<PointD> const & points);

  void Add(PointD const & p) { return Add(p.x, p.y); }
  void Add(double x, double y);

  bool HasPoint(PointD const & p) const { return HasPoint(p.x, p.y); }
  bool HasPoint(double x, double y) const;

  bool HasPoint(PointD const & p, double eps) const { return HasPoint(p.x, p.y, eps); }
  bool HasPoint(double x, double y, double eps) const;

  PointD Min() const { return m_min; }
  PointD Max() const { return m_max; }

  m2::RectD ToRect() const { return {Min(), Max()}; }

  std::vector<m2::PointD> Points() const
  {
    std::vector<m2::PointD> points(4);
    points[0] = Min();
    points[2] = Max();
    points[1] = PointD(points[2].x, points[0].y);
    points[3] = PointD(points[0].x, points[2].y);
    return points;
  }

  bool operator==(BoundingBox const & rhs) const { return m_min == rhs.m_min && m_max == rhs.m_max; }

  DECLARE_VISITOR(visitor(m_min, "min"), visitor(m_max, "max"))
  DECLARE_DEBUG_PRINT(BoundingBox)

private:
  // Infinity can not be used with -ffast-math
  static double constexpr kLargestDouble = std::numeric_limits<double>::max();
  static double constexpr kLowestDouble = std::numeric_limits<double>::lowest();

  m2::PointD m_min = m2::PointD(kLargestDouble, kLargestDouble);
  m2::PointD m_max = m2::PointD(kLowestDouble, kLowestDouble);
};
}  // namespace m2
