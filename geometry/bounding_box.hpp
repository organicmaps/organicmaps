#pragma once

#include "geometry/point2d.hpp"

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

  PointD Min() const { return m_min; }
  PointD Max() const { return m_max; }

  std::vector<m2::PointD> Points() const
  {
    std::vector<m2::PointD> points(4);
    points[0] = Min();
    points[2] = Max();
    points[1] = PointD(points[2].x, points[0].y);
    points[3] = PointD(points[0].x, points[2].y);
    return points;
  }

  bool operator==(BoundingBox const & rhs) const
  {
    return m_min == rhs.m_min && m_max == rhs.m_max;
  }

  DECLARE_VISITOR(visitor(m_min, "min"), visitor(m_max, "max"))
  DECLARE_DEBUG_PRINT(BoundingBox)

private:
  static_assert(std::numeric_limits<double>::has_infinity, "");
  static double constexpr kPositiveInfinity = std::numeric_limits<double>::infinity();
  static double constexpr kNegativeInfinity = -kPositiveInfinity;

  m2::PointD m_min = m2::PointD(kPositiveInfinity, kPositiveInfinity);
  m2::PointD m_max = m2::PointD(kNegativeInfinity, kNegativeInfinity);
};
}  // namespace m2
