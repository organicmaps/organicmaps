#include "routing/route_weight.hpp"

#include "routing/cross_mwm_connector.hpp"

#include <type_traits>

using namespace std;

namespace
{
template <typename Number, typename EnableIf = enable_if_t<is_integral<Number>::value, void>>
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
  if (m_numPassThroughChanges > 0 || m_numAccessChanges > 0)
    return connector::kNoRoute;
  return GetWeight();
}

RouteWeight RouteWeight::operator+(RouteWeight const & rhs) const
{
  ASSERT(!SumWillOverflow(m_numPassThroughChanges, rhs.m_numPassThroughChanges),
         (m_numPassThroughChanges, rhs.m_numPassThroughChanges));
  ASSERT(!SumWillOverflow(m_numAccessChanges, rhs.m_numAccessChanges),
         (m_numAccessChanges, rhs.m_numAccessChanges));
  ASSERT(!SumWillOverflow(m_numAccessConditionalPenalties, rhs.m_numAccessConditionalPenalties),
         (m_numAccessConditionalPenalties, rhs.m_numAccessConditionalPenalties));

  return RouteWeight(m_weight + rhs.m_weight, m_numPassThroughChanges + rhs.m_numPassThroughChanges,
                     m_numAccessChanges + rhs.m_numAccessChanges,
                     m_numAccessConditionalPenalties + rhs.m_numAccessConditionalPenalties,
                     m_transitTime + rhs.m_transitTime);
}

RouteWeight RouteWeight::operator-(RouteWeight const & rhs) const
{
  ASSERT_NOT_EQUAL(m_numPassThroughChanges, numeric_limits<int8_t>::min(), ());
  ASSERT_NOT_EQUAL(m_numAccessChanges, numeric_limits<int8_t>::min(), ());
  ASSERT(
      !SumWillOverflow(m_numPassThroughChanges, static_cast<int8_t>(-rhs.m_numPassThroughChanges)),
      (m_numPassThroughChanges, -rhs.m_numPassThroughChanges));
  ASSERT(!SumWillOverflow(m_numAccessChanges, static_cast<int8_t>(-rhs.m_numAccessChanges)),
         (m_numAccessChanges, -rhs.m_numAccessChanges));
  ASSERT(!SumWillOverflow(m_numAccessConditionalPenalties,
                          static_cast<int8_t>(-rhs.m_numAccessConditionalPenalties)),
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
  ASSERT(!SumWillOverflow(m_numAccessChanges, rhs.m_numAccessChanges),
         (m_numAccessChanges, rhs.m_numAccessChanges));
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
  // Prefer bigger transit time if total weights are same.
  return m_transitTime > rhs.m_transitTime;
}

/// @todo Introduce this integrated weight criteria into operator< to avoid strange situation when
/// short path but with m_numAccessChanges > 0 is worse than very long path but with m_numAccessChanges == 0.
/// Make some reasonable multiply factors with tests.
double RouteWeight::GetIntegratedWeight() const
{
  double res = m_weight;

  // +200% for each additional change.
  if (m_numPassThroughChanges)
    res += m_numPassThroughChanges * m_weight * 2;

  // +200% for each additional change.
  if (m_numAccessChanges)
    res += m_numAccessChanges * m_weight * 2;

  // +100% for each conditional.
  if (m_numAccessConditionalPenalties)
    res += m_numAccessConditionalPenalties * m_weight;

  return res;
}

ostream & operator<<(ostream & os, RouteWeight const & routeWeight)
{
  os << "("
     << static_cast<int32_t>(routeWeight.GetNumPassThroughChanges()) << ", "
     << static_cast<int32_t>(routeWeight.m_numAccessChanges) << ", "
     << static_cast<int32_t>(routeWeight.m_numAccessConditionalPenalties) << ", "
     << routeWeight.GetWeight() << ", " << routeWeight.GetTransitTime() << ")";
  return os;
}

RouteWeight operator*(double lhs, RouteWeight const & rhs)
{
  return RouteWeight(lhs * rhs.GetWeight(), rhs.GetNumPassThroughChanges(),
                     rhs.m_numAccessChanges, rhs.m_numAccessConditionalPenalties,
                     lhs * rhs.GetTransitTime());
}
}  // namespace routing
