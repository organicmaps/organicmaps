#include "routing/route_weight.hpp"

#include "routing/cross_mwm_connector.hpp"

#include <type_traits>

namespace
{
template <typename Number, typename EnableIf = std::enable_if_t<std::is_integral<Number>::value, void>>
bool SumWillOverflow(Number lhs, Number rhs)
{
  if (lhs > 0)
    return rhs > std::numeric_limits<Number>::max() - lhs;
  if (lhs < 0)
    return rhs < std::numeric_limits<Number>::min() - lhs;
  return false;
}
}  // namespace

namespace routing
{

int RouteWeight::s_PassThroughPenaltyS = 30 * 60;  // 30 minutes
int RouteWeight::s_AccessPenaltyS = 2 * 60 * 60;   // 2 hours

double RouteWeight::ToCrossMwmWeight() const
{
  // Do not accumulate pass-through or access-change edges into cross-mwm graph.
  /// @todo This is not very honest, but we have thousands of enter-exit edges
  /// in cross-mwm graph now and should filter them somehow.

  if (m_numPassThroughChanges > 0 || m_numAccessChanges > 0)
    return connector::kNoRoute;
  return GetWeight();
}

RouteWeight RouteWeight::operator+(RouteWeight const & rhs) const
{
  ASSERT(!SumWillOverflow(m_numPassThroughChanges, rhs.m_numPassThroughChanges),
         (m_numPassThroughChanges, rhs.m_numPassThroughChanges));
  ASSERT(!SumWillOverflow(m_numAccessChanges, rhs.m_numAccessChanges), (m_numAccessChanges, rhs.m_numAccessChanges));
  ASSERT(!SumWillOverflow(m_numAccessConditionalPenalties, rhs.m_numAccessConditionalPenalties),
         (m_numAccessConditionalPenalties, rhs.m_numAccessConditionalPenalties));

  return RouteWeight(m_weight + rhs.m_weight, m_numPassThroughChanges + rhs.m_numPassThroughChanges,
                     m_numAccessChanges + rhs.m_numAccessChanges,
                     m_numAccessConditionalPenalties + rhs.m_numAccessConditionalPenalties,
                     m_transitTime + rhs.m_transitTime);
}

RouteWeight RouteWeight::operator-(RouteWeight const & rhs) const
{
  ASSERT_NOT_EQUAL(m_numPassThroughChanges, std::numeric_limits<int8_t>::min(), ());
  ASSERT_NOT_EQUAL(m_numAccessChanges, std::numeric_limits<int8_t>::min(), ());
  ASSERT(!SumWillOverflow(m_numPassThroughChanges, static_cast<int8_t>(-rhs.m_numPassThroughChanges)),
         (m_numPassThroughChanges, -rhs.m_numPassThroughChanges));
  ASSERT(!SumWillOverflow(m_numAccessChanges, static_cast<int8_t>(-rhs.m_numAccessChanges)),
         (m_numAccessChanges, -rhs.m_numAccessChanges));
  ASSERT(!SumWillOverflow(m_numAccessConditionalPenalties, static_cast<int8_t>(-rhs.m_numAccessConditionalPenalties)),
         (m_numAccessConditionalPenalties, -rhs.m_numAccessConditionalPenalties));

  return RouteWeight(m_weight - rhs.m_weight, m_numPassThroughChanges - rhs.m_numPassThroughChanges,
                     m_numAccessChanges - rhs.m_numAccessChanges,
                     m_numAccessConditionalPenalties - rhs.m_numAccessConditionalPenalties,
                     m_transitTime - rhs.m_transitTime);
}

RouteWeight & RouteWeight::operator+=(RouteWeight const & rhs)
{
  ASSERT(!SumWillOverflow(m_numPassThroughChanges, rhs.m_numPassThroughChanges),
         (m_numPassThroughChanges, rhs.m_numPassThroughChanges));
  ASSERT(!SumWillOverflow(m_numAccessChanges, rhs.m_numAccessChanges), (m_numAccessChanges, rhs.m_numAccessChanges));
  ASSERT(!SumWillOverflow(m_numAccessConditionalPenalties, rhs.m_numAccessConditionalPenalties),
         (m_numAccessConditionalPenalties, rhs.m_numAccessConditionalPenalties));
  m_weight += rhs.m_weight;
  m_numPassThroughChanges += rhs.m_numPassThroughChanges;
  m_numAccessChanges += rhs.m_numAccessChanges;
  m_numAccessConditionalPenalties += rhs.m_numAccessConditionalPenalties;
  m_transitTime += rhs.m_transitTime;
  return *this;
}

bool RouteWeight::operator<(RouteWeight const & rhs) const
{
  /*
  if (m_numPassThroughChanges != rhs.m_numPassThroughChanges)
    return m_numPassThroughChanges < rhs.m_numPassThroughChanges;
  // We compare m_numAccessChanges after m_numPassThroughChanges because we can have multiple
  // nodes with access tags on the way from the area with limited access and no access tags on the
  // ways inside this area. So we probably need to make access restriction less strict than pass
  // through restrictions e.g. allow to cross access={private, destination} and build the route
  // with the least possible number of such crosses or introduce some maximal number of
  // access={private, destination} crosses.
  if (m_numAccessChanges != rhs.m_numAccessChanges)
    return m_numAccessChanges < rhs.m_numAccessChanges;

  if (m_numAccessConditionalPenalties != rhs.m_numAccessConditionalPenalties)
    return m_numAccessConditionalPenalties < rhs.m_numAccessConditionalPenalties;

  if (m_weight != rhs.m_weight)
    return m_weight < rhs.m_weight;
  */

  // The reason behind that is described here:
  // https://github.com/organicmaps/organicmaps/issues/1788
  // Before that change, a very small weight but with m_numPassThroughChanges > 0 was *always worse* than
  // a very big weight but with m_numPassThroughChanges == 0.
  auto const w1 = GetIntegratedWeight();
  auto const w2 = rhs.GetIntegratedWeight();
  if (w1 != w2)
    return w1 < w2;

  // Prefer bigger transit (on public transport) time if total weights are the same.
  return m_transitTime > rhs.m_transitTime;
}

double RouteWeight::GetIntegratedWeight() const
{
  double res = m_weight;

  if (m_numPassThroughChanges)
    res += m_numPassThroughChanges * s_PassThroughPenaltyS;

  if (m_numAccessChanges)
    res += m_numAccessChanges * s_AccessPenaltyS;

  if (m_numAccessConditionalPenalties)
    res += m_numAccessConditionalPenalties * s_AccessPenaltyS / 2;

  return res;
}

std::ostream & operator<<(std::ostream & os, RouteWeight const & routeWeight)
{
  os << "(" << static_cast<int32_t>(routeWeight.GetNumPassThroughChanges()) << ", "
     << static_cast<int32_t>(routeWeight.m_numAccessChanges) << ", "
     << static_cast<int32_t>(routeWeight.m_numAccessConditionalPenalties) << ", " << routeWeight.GetWeight() << ", "
     << routeWeight.GetTransitTime() << ")";
  return os;
}

RouteWeight operator*(double lhs, RouteWeight const & rhs)
{
  return RouteWeight(lhs * rhs.GetWeight(), rhs.GetNumPassThroughChanges(), rhs.m_numAccessChanges,
                     rhs.m_numAccessConditionalPenalties, lhs * rhs.GetTransitTime());
}
}  // namespace routing
