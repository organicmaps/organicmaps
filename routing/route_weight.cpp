#include "routing/route_weight.hpp"

#include "routing/cross_mwm_connector.hpp"

using namespace std;

namespace routing
{
double RouteWeight::ToCrossMwmWeight() const
{
  if (m_nonPassThroughCross > 0)
    return CrossMwmConnector::kNoRoute;
  return GetWeight();
}

ostream & operator<<(ostream & os, RouteWeight const & routeWeight)
{
  os << "(" << routeWeight.GetNonPassThroughCross() << ", " << routeWeight.GetWeight() << ")";
  return os;
}

RouteWeight operator*(double lhs, RouteWeight const & rhs)
{
  return RouteWeight(lhs * rhs.GetWeight(), rhs.GetNonPassThroughCross());
}
}  // namespace routing
