#include "testing/testing.hpp"

#include "routing/routing_integration_tests/generated_map_test.hpp"
#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "routing/routing_callbacks.hpp"
#include "routing/vehicle_mask.hpp"

#include "geometry/mercator.hpp"

#include <set>
#include <string>

namespace samples_route_tests
{
using namespace routing;

// Synthetic Novosibirsk fragment around Магаданская улица, see data/test_data/osm/nsk_magadanskaya.osm:
//   * way 51993176 "Магаданская улица" is highway=residential and connects start and finish directly;
//   * the parallel ways 154509134, 154534181, 154534187, 445068859, 445068868, 445068875 are highway=service.
// A car route between the two ends of Магаданская should simply follow the residential street, not
// detour through the parallel service roads (the start and finish project onto the service roads,
// so leaving/arriving them must not be penalized as a non-pass-through cut-through).
UNIT_TEST(Nsk_MagadanskayaResidentialRoute)
{
  std::string const mwmName = "Russia_Novosibirsk Oblast";
  integration::GeneratedMapTest map("./data/test_data/osm/nsk_magadanskaya.osm", mwmName, VehicleType::Car);

  auto const routeResult = integration::CalculateRoute(map.GetComponents(), mercator::FromLatLon(55.074347, 82.939134),
                                                       {0.0, 0.0}, mercator::FromLatLon(55.076779, 82.935965));
  TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());
  TEST(routeResult.first, ());

  std::set<uint64_t> const usedWays = map.GetUsedOsmWays(*routeResult.first);
  TEST(usedWays.count(51993176) > 0, ("Car route should follow residential way 51993176, but used ways:", usedWays));
}
}  // namespace samples_route_tests
