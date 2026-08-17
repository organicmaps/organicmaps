#include "testing/testing.hpp"

#include "routing/route_weight.hpp"

namespace route_weight_tests
{
using routing::RouteWeight;

// IndexGraphStarter credits the mandatory "leave start / arrive finish" crossings when an ending
// lies in a non-pass-through (living/service) zone, which can drive m_numPassThroughChanges below
// zero. Such a credited weight must not be cheaper (on the pass-through component) than a plain one:
// only a positive remainder — a genuine cut-through — is penalized.
UNIT_TEST(RouteWeight_PassThroughCreditIsClamped)
{
  double const w = 100.0;
  int const penalty = RouteWeight::s_PassThroughPenaltyS;

  // No genuine pass-through: a fully credited route (e.g. an all-service route between two service
  // endings, crossings 0, credit -2) is not penalized and not made artificially cheap.
  TEST_EQUAL(RouteWeight(w, 0 /* numPassThroughChanges */, 0, 0, 0.0).GetIntegratedWeight(), w, ());
  TEST_EQUAL(RouteWeight(w, -1, 0, 0, 0.0).GetIntegratedWeight(), w, ());
  TEST_EQUAL(RouteWeight(w, -2, 0, 0, 0.0).GetIntegratedWeight(), w, ());

  // A genuine cut-through (positive remainder after crediting) is still penalized.
  TEST_EQUAL(RouteWeight(w, 1, 0, 0, 0.0).GetIntegratedWeight(), w + penalty, ());
  TEST_EQUAL(RouteWeight(w, 2, 0, 0, 0.0).GetIntegratedWeight(), w + 2 * penalty, ());

  // Crossing-only route (credit -2) ties on the pass-through penalty with a route that has no
  // crossings, so a shorter such route wins purely on time.
  TEST(RouteWeight(90.0, -2, 0, 0, 0.0) < RouteWeight(95.0, 0, 0, 0, 0.0), ());
  // ... but a genuine cut-through stays worse than a slightly longer clean route.
  TEST(RouteWeight(95.0, 0, 0, 0, 0.0) < RouteWeight(90.0, 1, 0, 0, 0.0), ());
}
}  // namespace route_weight_tests
