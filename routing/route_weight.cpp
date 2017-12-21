#include "routing/route_weight.hpp"

#include "routing/cross_mwm_connector.hpp"

#include <type_traits>

using namespace std;

namespace
{
template <typename Number,
          typename EnableIf = typename enable_if<is_integral<Number>::value, void>::type>
bool SumWillOverflow(Number lhs, Number rhs)
{
  if (lhs > 0)
    return rhs > numeric_limits<Number>::max() - lhs;
  if (lhs < 0)
    return rhs < numeric_limits<Number>::min() - lhs;
  return false;
}
}  // namespace

namespace routing
{
double RouteWeight::ToCrossMwmWeight() const
{
  if (m_nonPassThroughCross > 0 || m_numAccessChanges > 0)
    return connector::kNoRoute;
  return GetWeight();
}

RouteWeight RouteWeight::operator+(RouteWeight const & rhs) const
{
  ASSERT(!SumWillOverflow(m_nonPassThroughCross, rhs.m_nonPassThroughCross),
         (m_nonPassThroughCross, rhs.m_nonPassThroughCross));
  ASSERT(!SumWillOverflow(m_numAccessChanges, rhs.m_numAccessChanges),
         (m_numAccessChanges, rhs.m_numAccessChanges));
  return RouteWeight(m_weight + rhs.m_weight, m_nonPassThroughCross + rhs.m_nonPassThroughCross,
                     m_numAccessChanges + rhs.m_numAccessChanges,
                     m_transitTime + rhs.m_transitTime);
}

RouteWeight RouteWeight::operator-(RouteWeight const & rhs) const
{
  ASSERT_NOT_EQUAL(m_nonPassThroughCross, std::numeric_limits<int32_t>::min(), ());
  ASSERT_NOT_EQUAL(m_numAccessChanges, std::numeric_limits<int32_t>::min(), ());
  ASSERT(!SumWillOverflow(m_nonPassThroughCross, -rhs.m_nonPassThroughCross),
         (m_nonPassThroughCross, -rhs.m_nonPassThroughCross));
  ASSERT(!SumWillOverflow(m_numAccessChanges, -rhs.m_numAccessChanges),
         (m_numAccessChanges, -rhs.m_numAccessChanges));
  return RouteWeight(m_weight - rhs.m_weight, m_nonPassThroughCross - rhs.m_nonPassThroughCross,
                     m_numAccessChanges - rhs.m_numAccessChanges,
                     m_transitTime - rhs.m_transitTime);
}

RouteWeight & RouteWeight::operator+=(RouteWeight const & rhs)
{
  ASSERT(!SumWillOverflow(m_nonPassThroughCross, rhs.m_nonPassThroughCross),
         (m_nonPassThroughCross, rhs.m_nonPassThroughCross));
  ASSERT(!SumWillOverflow(m_numAccessChanges, rhs.m_numAccessChanges),
         (m_numAccessChanges, rhs.m_numAccessChanges));
  m_weight += rhs.m_weight;
  m_nonPassThroughCross += rhs.m_nonPassThroughCross;
  m_numAccessChanges += rhs.m_numAccessChanges;
  m_transitTime += rhs.m_transitTime;
  return *this;
}

ostream & operator<<(ostream & os, RouteWeight const & routeWeight)
{
  os << "(" << routeWeight.GetNonPassThroughCross() << ", " << routeWeight.GetNumAccessChanges()
     << ", " << routeWeight.GetWeight() << ", " << routeWeight.GetTransitTime() << ")";
  return os;
}

RouteWeight operator*(double lhs, RouteWeight const & rhs)
{
  return RouteWeight(lhs * rhs.GetWeight(), rhs.GetNonPassThroughCross(), rhs.GetNumAccessChanges(),
                     lhs * rhs.GetTransitTime());
}
}  // namespace routing
