#pragma once

#include "geometry/point2d.hpp"

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

  PointD Min() const { return PointD(m_minX, m_minY); }
  PointD Max() const { return PointD(m_maxX, m_maxY); }

private:
  static_assert(std::numeric_limits<double>::has_infinity, "");
  static double constexpr kPositiveInfinity = std::numeric_limits<double>::infinity();
  static double constexpr kNegativeInfinity = -kPositiveInfinity;

  double m_minX = kPositiveInfinity;
  double m_minY = kPositiveInfinity;
  double m_maxX = kNegativeInfinity;
  double m_maxY = kNegativeInfinity;
};
}  // namespace m2
