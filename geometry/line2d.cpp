#include "geometry/line2d.hpp"

#include <cmath>
#include <sstream>

using namespace std;

namespace m2
{
namespace
{
bool Collinear(PointD const & a, PointD const & b, double eps)
{
  return fabs(CrossProduct(a, b)) < eps;
}
}  // namespace

string DebugPrint(Line2D const & line)
{
  ostringstream os;
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

string DebugPrint(IntersectionResult::Type type)
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

string DebugPrint(IntersectionResult const & result)
{
  ostringstream os;
  os << "Result [";
  if (result.m_type == IntersectionResult::Type::One)
    os << DebugPrint(result.m_point);
  else
    os << DebugPrint(result.m_type);
  os << "]";
  return os.str();
}
}  // namespace m2
