#include "testing/testing.hpp"

#include "routing/route.hpp"

#include <utility>
#include <vector>

namespace routing
{
UNIT_TEST(RoutesResult_SelectFastestPreservesTies)
{
  RoutesResult result;
  struct TestRoute : RouteBase
  {
    using RouteBase::SetRouteSegments;
  };
  for (auto const & [distance, seconds] : std::vector<std::pair<double, double>>{{1000, 600}, {2000, 300}, {1500, 300}})
  {
    TestRoute route;
    RouteSegment segment({}, {}, {}, {});
    segment.SetDistancesAndTime(distance, distance, seconds);
    route.SetRouteSegments({segment});
    result.m_routes.push_back(std::move(route));
  }
  TEST_EQUAL(result.GetFastestRouteIndex(), 1, ());
  result.m_activeIdx = 2;
  TEST_EQUAL(result.GetFastestRouteIndex(), 2, ());
  result.m_routes.resize(1);
  result.m_activeIdx = 0;
  TEST_EQUAL(result.GetFastestRouteIndex(), 0, ());
}

}  // namespace routing
