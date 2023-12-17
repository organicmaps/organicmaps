#pragma once

#include "geometry/point2d.hpp"

#include "base/visitor.hpp"

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
  // 1e-12 is used here because of we are going to use the box on
  // Mercator plane, where the precision of all coords is 1e-5, so we
  // are off by two orders of magnitude from the precision of data.
  static double constexpr kEps = 1e-12;

  CalipersBox() = default;
  explicit CalipersBox(std::vector<PointD> const & points);

  // Used in CitiesBoundariesDecoder. Faster than ctor.
  void Deserialize(std::vector<PointD> && points);
  // Remove useless points in case of degenerate box. Used in unit tests.
  void Normalize();

  bool TestValid() const;

  std::vector<PointD> const & Points() const { return m_points; }

  bool HasPoint(PointD const & p) const { return HasPoint(p, kEps); }
  bool HasPoint(double x, double y) const { return HasPoint(m2::PointD(x, y)); }

  bool HasPoint(PointD const & p, double eps) const;
  bool HasPoint(double x, double y, double eps) const { return HasPoint(PointD(x, y), eps); }

  bool operator==(CalipersBox const & rhs) const { return m_points == rhs.m_points; }

  DECLARE_VISITOR(visitor(m_points, "points"))
  DECLARE_DEBUG_PRINT(CalipersBox)

private:
  std::vector<PointD> m_points;
};
}  // namespace m2
