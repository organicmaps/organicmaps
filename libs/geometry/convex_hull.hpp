#pragma once

#include "geometry/point2d.hpp"
#include "geometry/segment2d.hpp"

#include "base/assert.hpp"

#include <vector>

namespace m2
{
class ConvexHull
{
public:
  // Builds a convex hull around |points|.  The hull polygon points are
  // listed in the order of a counterclockwise traversal with no three
  // points lying on the same straight line.
  //
  // Complexity: O(n * log(n)), where n is the number of points.
  ConvexHull(std::vector<PointD> const & points, double eps);

  size_t Size() const { return m_hull.size(); }
  bool Empty() const { return m_hull.empty(); }

  PointD const & PointAt(size_t i) const
  {
    ASSERT(!Empty(), ());
    return m_hull[i % Size()];
  }

  Segment2D SegmentAt(size_t i) const
  {
    ASSERT_GREATER_OR_EQUAL(Size(), 2, ());
    return Segment2D(PointAt(i), PointAt(i + 1));
  }

  std::vector<PointD> const & Points() const { return m_hull; }

private:
  std::vector<PointD> m_hull;
};
}  // namespace m2
