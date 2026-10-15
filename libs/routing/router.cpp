#include "routing/router.hpp"

#include "geometry/mercator.hpp"

namespace routing
{
bool IRouter::IsRoadBehind(m2::PointD const & point, m2::PointD const & roadPoint, m2::PointD const & direction)
{
  // The end of the piece the car has just passed, the next piece goes on from it.
  double constexpr kBehindM = 0.5;
  m2::PointD const shift = roadPoint - point;
  if (shift.IsAlmostZero() || direction.IsAlmostZero())
    return false;
  double const cosAngle = m2::DotProduct(shift, direction) / (shift.Length() * direction.Length());
  return mercator::DistanceOnEarth(point, roadPoint) * cosAngle < -kBehindM;
}

std::string ToString(RouterType type)
{
  switch (type)
  {
  case RouterType::Vehicle: return "vehicle";
  case RouterType::Pedestrian: return "pedestrian";
  case RouterType::Bicycle: return "bicycle";
  case RouterType::Transit: return "transit";
  case RouterType::Ruler: return "ruler";
  case RouterType::Count: return "count";
  }
  ASSERT(false, ());
  return "Error";
}

RouterType FromString(std::string const & str)
{
  if (str == "vehicle")
    return RouterType::Vehicle;
  if (str == "pedestrian")
    return RouterType::Pedestrian;
  if (str == "bicycle")
    return RouterType::Bicycle;
  if (str == "transit")
    return RouterType::Transit;
  if (str == "ruler")
    return RouterType::Ruler;

  ASSERT(false, ("Incorrect routing string:", str));
  return RouterType::Vehicle;
}

std::string DebugPrint(RouterType type)
{
  return ToString(type);
}
}  //  namespace routing
