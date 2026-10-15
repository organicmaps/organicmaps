#include "testing/testing.hpp"

#include "map/search_api.hpp"

#include "geometry/mercator.hpp"

#include <string>
#include <vector>

namespace search_nearest_first_tests
{
using namespace search;

m2::PointD const kPivot = mercator::FromLatLon(0, 0);

// A result this far east of the pivot.
Result MakeResult(std::string const & name, double eastKm)
{
  Result result(mercator::FromLatLon(0, eastKm / 111.2), name);
  result.SetType(Result::Type::Feature);
  return result;
}

std::vector<std::string> Names(Results const & results)
{
  std::vector<std::string> names;
  for (auto const & result : results)
    names.push_back(result.GetString());
  return names;
}

UNIT_TEST(OrderNearestFirst_KeepsPlacesFirst)
{
  Results results;
  results.AddResultNoChecks(Result("street", "street "));
  results.AddResultNoChecks(MakeResult("street far", 30));
  results.AddResultNoChecks(MakeResult("town", 50));
  results.AddResultNoChecks(MakeResult("street near", 1));
  results.AddResultNoChecks(MakeResult("street middle", 5));
  results.AddResultNoChecks(MakeResult("shop near", 1));
  results.AddResultNoChecks(MakeResult("village", 20));

  OrderNearestFirst(kPivot, [](Result const & r) { return r.GetString() == "town" || r.GetString() == "village"; },
                    results);
  // The suggestion and the places stay first in their order, the others are nearest first, the equally near ones
  // in their order.
  TEST_EQUAL(Names(results),
             (std::vector<std::string>{"street", "town", "village", "street near", "shop near", "street middle",
                                       "street far"}),
             ());
  for (size_t i = 0; i < results.GetCount(); ++i)
    TEST_EQUAL(results[i].GetPositionInResults(), static_cast<int32_t>(i), ());
}
}  // namespace search_nearest_first_tests
