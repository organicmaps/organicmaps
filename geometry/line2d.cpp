#include "geometry/line2d.hpp"

#include <cmath>
#include <sstream>

namespace m2
{
namespace
{
bool Collinear(PointD const & a, PointD const & b, double eps)
{
  return std::fabs(CrossProduct(a, b)) < eps;
}
}  // namespace

std::string DebugPrint(Line2D const & line)
{
  std::ostringstream os;
  os << "Line2D [ ";
  os << "point: " << DebugPrint(line.m_point) << ", ";
  os << "direction: " << DebugPrint(line.m_direction);
  os << " ]";
  return os.str();
}

IntersectionResult Intersect(Line2D const & lhs, Line2D const & rhs, double eps)
{
  auto const & a = lhs.m_point;
  auto const & ab = lhs.m_direction;

  auto const & c = rhs.m_point;
  auto const & cd = rhs.m_direction;

  if (Collinear(ab, cd, eps))
  {
    if (Collinear(c - a, cd, eps))
      return IntersectionResult(IntersectionResult::Type::Infinity);
    return IntersectionResult(IntersectionResult::Type::Zero);
  }

  auto const ac = c - a;

  auto const n = CrossProduct(ac, cd);
  auto const d = CrossProduct(ab, cd);
  auto const scale = n / d;

  return IntersectionResult(a + ab * scale);
}
}  // namespace m2
