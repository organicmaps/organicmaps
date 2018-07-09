#include "geometry/segment2d.hpp"

#include "geometry/robust_orientation.hpp"

#include <algorithm>

using namespace std;

namespace m2
{
string DebugPrint(Segment2D const & segment)
{
  return "(" + DebugPrint(segment.m_u) + ", " + DebugPrint(segment.m_v) + ")";
}

bool IsPointOnSegmentEps(PointD const & pt, PointD const & p1, PointD const & p2, double eps)
{
  double const t = robust::OrientedS(p1, p2, pt);

  if (fabs(t) > eps)
    return false;

  double minX = p1.x;
  double maxX = p2.x;
  if (maxX < minX)
    swap(maxX, minX);

  double minY = p1.y;
  double maxY = p2.y;
  if (maxY < minY)
    swap(maxY, minY);

  return pt.x >= minX - eps && pt.x <= maxX + eps && pt.y >= minY - eps && pt.y <= maxY + eps;
}

bool IsPointOnSegment(PointD const & pt, PointD const & p1, PointD const & p2)
{
  // The epsilon here is chosen quite arbitrarily, to pass paranoid
  // tests and to match our real-data geometry precision. If you have
  // better ideas how to check whether pt belongs to (p1, p2) segment
  // more precisely or without kEps, feel free to submit a pull
  // request.
  double const kEps = 1e-100;
  return IsPointOnSegmentEps(pt, p1, p2, kEps);
}
}  // namespace m2
