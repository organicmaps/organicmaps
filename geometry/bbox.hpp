#pragma once

#include "geometry/point2d.hpp"

#include <limits>
#include <vector>

namespace m2
{
class BBox
{
public:
  BBox() = default;
  BBox(std::vector<m2::PointD> const & points);

  void Add(m2::PointD const & p) { return Add(p.x, p.y); }
  void Add(double x, double y);

  bool IsInside(m2::PointD const & p) const { return IsInside(p.x, p.y); }
  bool IsInside(double x, double y) const;

  m2::PointD Min() const { return m2::PointD(m_minX, m_minY); }
  m2::PointD Max() const { return m2::PointD(m_maxX, m_maxY); }

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
