#include "testing/testing.hpp"

#include "routing/routing_integration_tests/generated_map_test.hpp"
#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "routing/index_router.hpp"
#include "routing/route.hpp"
#include "routing/routing_callbacks.hpp"
#include "routing/vehicle_mask.hpp"

#include "geometry/mercator.hpp"

#include <cstdint>
#include <set>
#include <string>

namespace routing
{
struct IndexRouterTest
{
  static RouterResultCode AdjustRoute(IndexRouter & router, Checkpoints const & checkpoints, Route & route)
  {
    RouterDelegate delegate;
    return router.AdjustRoute(checkpoints, {} /* startDirection */, delegate, route);
  }
};
}  // namespace routing

namespace samples_route_tests
{
using namespace routing;

// Synthetic Novosibirsk fragment around Магаданская улица, see data/test_data/osm/nsk_magadanskaya.osm:
//   * way 51993176 "Магаданская улица" is highway=residential and connects start and finish directly;
//   * the parallel ways 154509134, 154534181, 154534187, 445068859, 445068868, 445068875 are highway=service.
// A car route between the two ends of Магаданская should simply follow the residential street, not
// detour through the parallel service roads (the start and finish project onto the service roads,
// so leaving/arriving them must not be penalized as a non-pass-through cut-through).
UNIT_TEST(Sample_Nsk_MagadanskayaResidentialRoute)
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

// Rejoining the old initial service episode would turn its free exit into an interior cut-through.
UNIT_TEST(Sample_Nsk_PublicStartAfterServiceRerouting)
{
  integration::GeneratedMapTest map("./data/test_data/osm/service_rerouting.osm", "Russia_Novosibirsk Oblast",
                                    VehicleType::Car);
  auto & router = static_cast<IndexRouter &>(map.GetComponents().GetRouter());
  auto const serviceStart = mercator::FromLatLon(55.001, 83.0);
  auto const publicStart = mercator::FromLatLon(55.0, 82.99);
  auto const finish = mercator::FromLatLon(55.002, 83.015);
  uint64_t constexpr kInitialServiceWay = 3;

  for (bool const cachedServiceStart : {false, true})
  {
    router.ClearState();
    RouterDelegate delegate;
    RoutesResult previous;
    TEST_EQUAL(router.CalculateRoute(Checkpoints(cachedServiceStart ? serviceStart : publicStart, finish), {},
                                     false /* adjustToPrevRoute */, false /* needAlternatives */, delegate, previous),
               RouterResultCode::NoError, (cachedServiceStart));
    TEST(previous.IsValid(), ());
    TEST_EQUAL(map.GetUsedOsmWays(Route(previous.GetActive())).contains(kInitialServiceWay), cachedServiceStart, ());

    // Exercise adjustment directly: rejecting every adjustment must not satisfy either case.
    Route adjusted;
    TEST_EQUAL(IndexRouterTest::AdjustRoute(router, Checkpoints(publicStart, finish), adjusted),
               RouterResultCode::NoError, (cachedServiceStart));
    TEST(adjusted.IsValid(), ());
    auto const usedWays = map.GetUsedOsmWays(adjusted);
    TEST(!usedWays.contains(kInitialServiceWay), (cachedServiceStart, usedWays));
    TEST(usedWays.contains(4) && usedWays.contains(5), (cachedServiceStart, usedWays));
  }
}
}  // namespace samples_route_tests
