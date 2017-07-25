#include "routing/route_weight.hpp"

using namespace std;

namespace routing
{
ostream & operator<<(ostream & os, RouteWeight const & routeWeight)
{
  os << routeWeight.GetWeight();
  return os;
}

RouteWeight operator*(double lhs, RouteWeight const & rhs)
{
  return RouteWeight(lhs * rhs.GetWeight());
}
}  // namespace routing
