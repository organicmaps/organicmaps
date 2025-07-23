#include "geometry/segment2d.hpp"

#include "geometry/line2d.hpp"
#include "geometry/robust_orientation.hpp"

#include <algorithm>
#include <sstream>
#include <string>

namespace m2
{
bool IsPointOnSegmentEps(PointD const & pt, PointD const & p1, PointD const & p2, double eps)
{
  double const t = robust::OrientedS(p1, p2, pt);

  if (std::fabs(t) > eps)
    return false;

  double minX = p1.x;
  double maxX = p2.x;
  if (maxX < minX)
    std::swap(maxX, minX);

  double minY = p1.y;
  double maxY = p2.y;
  if (maxY < minY)
    std::swap(maxY, minY);

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

bool SegmentsIntersect(PointD const & a, PointD const & b, PointD const & c, PointD const & d)
{
  using std::max, std::min;
  return max(a.x, b.x) >= min(c.x, d.x) && min(a.x, b.x) <= max(c.x, d.x) && max(a.y, b.y) >= min(c.y, d.y) &&
         min(a.y, b.y) <= max(c.y, d.y) && robust::OrientedS(a, b, c) * robust::OrientedS(a, b, d) <= 0.0 &&
         robust::OrientedS(c, d, a) * robust::OrientedS(c, d, b) <= 0.0;
}

IntersectionResult Intersect(Segment2D const & seg1, Segment2D const & seg2, double eps)
{
  if (!SegmentsIntersect(seg1.m_u, seg1.m_v, seg2.m_u, seg2.m_v))
    return IntersectionResult(IntersectionResult::Type::Zero);

  Line2D const line1(seg1);
  Line2D const line2(seg2);
  auto const lineIntersection = Intersect(line1, line2, eps);
  if (lineIntersection.m_type != IntersectionResult::Type::One)
    return lineIntersection;

  if (IsPointOnSegmentEps(lineIntersection.m_point, seg1.m_u, seg1.m_v, eps) &&
      IsPointOnSegmentEps(lineIntersection.m_point, seg2.m_u, seg2.m_v, eps))
  {
    return lineIntersection;
  }

  return IntersectionResult(IntersectionResult::Type::Zero);
}

std::string DebugPrint(Segment2D const & segment)
{
  return "(" + DebugPrint(segment.m_u) + ", " + DebugPrint(segment.m_v) + ")";
}

std::string DebugPrint(IntersectionResult::Type type)
{
  using Type = IntersectionResult::Type;

  switch (type)
  {
  case Type::Zero: return "Zero";
  case Type::One: return "One";
  case Type::Infinity: return "Infinity";
  }
  UNREACHABLE();
}

std::string DebugPrint(IntersectionResult const & result)
{
  std::ostringstream os;
  os << "Result [";
  if (result.m_type == IntersectionResult::Type::One)
    os << DebugPrint(result.m_point);
  else
    os << DebugPrint(result.m_type);
  os << "]";
  return os.str();
}
}  // namespace m2
