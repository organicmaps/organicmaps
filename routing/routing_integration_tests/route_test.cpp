#include "testing/testing.hpp"

#include "routing/routing_callbacks.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "geometry/mercator.hpp"

#include "std/limits.hpp"

using namespace routing;

namespace
{
  UNIT_TEST(StrangeCaseInAfrica)
  {
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(19.20789, 30.50663), {0., 0.},
        MercatorBounds::FromLatLon(19.17289, 30.47315), 7645.0);
  }

  UNIT_TEST(MoscowKashirskoeShosseCrossing)
  {
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(55.66216, 37.63259), {0., 0.},
        MercatorBounds::FromLatLon(55.66237, 37.63560), 2320.);
  }

  UNIT_TEST(MoscowToSVOAirport)
  {
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(55.75100, 37.61790), {0., 0.},
        MercatorBounds::FromLatLon(55.97310, 37.41460), 37284.);
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(55.97310, 37.41460), {0., 0.},
        MercatorBounds::FromLatLon(55.75100, 37.61790), 39449.);
  }

  // Restrictions tests. Check restrictions generation, if there are any errors.
  UNIT_TEST(RestrictionTestNeatBaumanAndTTK)
  {
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(55.77399, 37.68468), {0., 0.},
        MercatorBounds::FromLatLon(55.77198, 37.68782), 1032.);
  }

  UNIT_TEST(RestrictionTestNearMetroShodnenskaya)
  {
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(55.85043, 37.43824), {0., 0.},
        MercatorBounds::FromLatLon(55.85191, 37.43910), 510.);
  }

  // Strange asserts near Cupertino test
  UNIT_TEST(CaliforniaCupertinoFindPhantomAssertTest)
  {
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(37.33409, -122.03458), {0., 0.},
        MercatorBounds::FromLatLon(37.33498, -122.03575), 1438.);
  }

  // Path in the last map through the other map.
  UNIT_TEST(RussiaUfaToUstKatavTest)
  {
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(54.7304, 55.9554), {0., 0.},
        MercatorBounds::FromLatLon(54.9228, 58.1469), 164667.);
  }

  UNIT_TEST(RussiaMoscowNoServiceCrossing)
  {
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(55.77787, 37.70405), {0., 0.},
        MercatorBounds::FromLatLon(55.77682, 37.70391), 3140.);
  }

  UNIT_TEST(RussiaMoscowShortWayToService)
  {
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(55.77787, 37.70405), {0., 0.},
        MercatorBounds::FromLatLon(55.77691, 37.70428), 150.);
  }

  UNIT_TEST(PriceIslandLoadCrossGeometryTest)
  {
    size_t constexpr kExpectedPointsNumber = 56;
    // Forward
    TRouteResult route =
        integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                    MercatorBounds::FromLatLon(46.16255, -63.81643), {0., 0.},
                                    MercatorBounds::FromLatLon(46.25401, -63.70213));
    TEST_EQUAL(route.second, RouterResultCode::NoError, ());
    CHECK(route.first, ());
    integration::TestRoutePointsNumber(*route.first, kExpectedPointsNumber);
    // And backward case
    route = integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                        MercatorBounds::FromLatLon(46.25401, -63.70213), {0., 0.},
                                        MercatorBounds::FromLatLon(46.16255, -63.81643));
    TEST_EQUAL(route.second, RouterResultCode::NoError, ());
    CHECK(route.first, ());
    integration::TestRoutePointsNumber(*route.first, kExpectedPointsNumber);
  }

  // Cross mwm tests.
  UNIT_TEST(RussiaMoscowLeningradskiy39GerPanfilovtsev22RouteTest)
  {
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        {37.53758809983519, 67.536162466434234}, {0., 0.}, {37.40993977728661, 67.644784047393685},
        14296.);
  }

  UNIT_TEST(NederlandLeeuwardenToDenOeverTest)
  {
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(53.2076, 5.7082), {0., 0.},
        MercatorBounds::FromLatLon(52.9337, 5.0308), 59500.);
  }

  UNIT_TEST(RussiaMoscowGerPanfilovtsev22SolodchaPravdiRouteTest)
  {
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        {37.409929478750627, 67.644798619710073}, {0., 0.},
        {39.836562407458047, 65.774372510437971}, 239426.);
  }

  UNIT_TEST(RussiaMoscowBelarusMinsk)
  {
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(55.750650, 37.617673), {0., 0.},
        MercatorBounds::FromLatLon(53.902114, 27.562020), 712649.0);
  }

  UNIT_TEST(UKRugbyStIvesRouteTest)
  {
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        {-1.2653036222483705, 61.691304855049886}, {0., 0.},
        {-5.4799407508360218, 58.242809563579847}, 455902.);
  }

  UNIT_TEST(RussiaMoscowLenigradskiy39ItalySienaCenterRouteTest)
  {
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        {37.537596024929826, 67.536160359657288}, {0., 0.},
        {11.327927635052676, 48.166256203616726}, 2870710.);
  }

  UNIT_TEST(PeruSingleRoadTest)
  {
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(-14.22061, -73.35969), {0., 0.},
        MercatorBounds::FromLatLon(-14.22389, -73.44281), 15900.);
  }

  UNIT_TEST(RussiaMoscowFranceParisCenterRouteTest)
  {
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(55.75271, 37.62618), {0., 0.},
        MercatorBounds::FromLatLon(48.86123, 2.34129), 2840940.);
  }

  UNIT_TEST(EnglandToFranceRouteLeMansTest)
  {
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(51.09276, 1.11369), {0., 0.},
        MercatorBounds::FromLatLon(50.93227, 1.82725), 60498.);
  }

  UNIT_TEST(RussiaMoscowStartAtTwowayFeatureTest)
  {
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(55.771, 37.5184), {0., 0.},
        MercatorBounds::FromLatLon(55.7718, 37.5178), 147.4);
  }

  UNIT_TEST(RussiaMoscowRegionToBelarusBorder)
  {
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(55.464182, 35.943947), {0.0, 0.0},
        MercatorBounds::FromLatLon(52.442467, 31.609642), 554000.);
  }

  UNIT_TEST(GermanyToTallinCrossMwmRoute)
  {
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(48.397416, 16.515289), {0.0, 0.0},
        MercatorBounds::FromLatLon(59.437214, 24.745355), 1650000.);
  }

  // Strange map edits in Africa borders. Routing not linked now.
  /*
  UNIT_TEST(RussiaMoscowLenigradskiy39RepublicOfSouthAfricaCapeTownCenterRouteTest)
  {
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(55.79721, 37.53786), {0., 0.},
              MercatorBounds::FromLatLon(-33.9286, 18.41837), 13701400.0);
  }
  */

  UNIT_TEST(MoroccoToSahrawiCrossMwmTest)
  {
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(27.15587, -13.23059), {0., 0.},
        MercatorBounds::FromLatLon(27.94049, -12.88800), 100864);
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(27.94049, -12.88800), {0., 0.},
        MercatorBounds::FromLatLon(27.15587, -13.23059), 100864);
  }

  UNIT_TEST(AlbaniaToMontenegroCrossTest)
  {
    // Road from Albania to Montenegro. Test turnaround finding at border (when start/stop
    // points are inside borders and one of segments has outside points).
    // Forward
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(42.01535, 19.40044), {0., 0.},
        MercatorBounds::FromLatLon(42.01201, 19.36286), 3674.);
    // And backward case
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(42.01201, 19.36286), {0., 0.},
        MercatorBounds::FromLatLon(42.01535, 19.40044), 3674.);
  }

  UNIT_TEST(CanadaBridgeCrossToEdwardIsland)
  {
    // Forward
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(46.13418, -63.84656), {0., 0.},
        MercatorBounds::FromLatLon(46.26739, -63.63907), 23000.);
    // And backward case
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(46.26739, -63.63907), {0., 0.},
        MercatorBounds::FromLatLon(46.13418, -63.84656), 23000.);
  }

  UNIT_TEST(RussiaFerryToCrimea)
  {
    // Forward
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(45.38053, 36.73226), {0., 0.},
        MercatorBounds::FromLatLon(45.36078, 36.60866), 15500.);
    // And backward case
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(45.36078, 36.60866), {0., 0.},
        MercatorBounds::FromLatLon(45.38053, 36.73226), 15500.);
  }

  UNIT_TEST(ParisCrossDestinationInForwardHeapCase)
  {
    // Forward
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(49.85015, 2.24296), {0., 0.},
        MercatorBounds::FromLatLon(48.85458, 2.36291), 127162.0);
    // And backward case
    integration::CalculateRouteAndTestRouteLength(
        integration::GetVehicleComponents<VehicleType::Car>(),
        MercatorBounds::FromLatLon(48.85458, 2.36291), {0., 0.},
        MercatorBounds::FromLatLon(49.85015, 2.24296), 137009.0);
  }

  UNIT_TEST(RussiaSmolenskRussiaMoscowTimeTest)
  {
    TRouteResult const routeResult =
        integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                    MercatorBounds::FromLatLon(54.7998, 32.05489), {0., 0.},
                                    MercatorBounds::FromLatLon(55.753, 37.60169));

    RouterResultCode const result = routeResult.second;
    TEST_EQUAL(result, RouterResultCode::NoError, ());

    CHECK(routeResult.first, ());
    Route const & route = *routeResult.first;
    integration::TestRouteTime(route, 16594.5);
  }

  UNIT_TEST(RussiaMoscowLenigradskiy39GeroevPanfilovtsev22TimeTest)
  {
    TRouteResult const routeResult =
        integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                    MercatorBounds::FromLatLon(55.7971, 37.53804), {0., 0.},
                                    MercatorBounds::FromLatLon(55.8579, 37.40990));
    RouterResultCode const result = routeResult.second;
    TEST_EQUAL(result, RouterResultCode::NoError, ());

    CHECK(routeResult.first, ());
    Route const & route = *routeResult.first;
    integration::TestRouteTime(route, 955.5);
  }

  UNIT_TEST(RussiaMoscowLenigradskiy39GeroevPanfilovtsev22SubrouteTest)
  {
    TRouteResult const routeResult =
        integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                    MercatorBounds::FromLatLon(55.7971, 37.53804), {0., 0.},
                                    MercatorBounds::FromLatLon(55.8579, 37.40990));

    RouterResultCode const result = routeResult.second;
    TEST_EQUAL(result, RouterResultCode::NoError, ());

    CHECK(routeResult.first, ());
    Route const & route = *routeResult.first;
    TEST_EQUAL(route.GetSubrouteCount(), 1, ());
    vector<RouteSegment> info;
    route.GetSubrouteInfo(0, info);
    TEST_EQUAL(route.GetPoly().GetSize(), info.size() + 1, ());
    size_t constexpr kExpectedPointsNumber = 335;
    integration::TestRoutePointsNumber(route, kExpectedPointsNumber);
  }

  UNIT_TEST(USALosAnglesAriaTwentyninePalmsHighwayTimeTest)
  {
    TRouteResult const routeResult =
        integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                    MercatorBounds::FromLatLon(34.0739, -115.3212), {0.0, 0.0},
                                    MercatorBounds::FromLatLon(34.0928, -115.5930));
    RouterResultCode const result = routeResult.second;
    TEST_EQUAL(result, RouterResultCode::NoError, ());
    CHECK(routeResult.first, ());
    Route const & route = *routeResult.first;
    TEST_LESS(route.GetTotalTimeSec(), numeric_limits<double >::max() / 2.0, ());
  }

  // Test on routing along features with tag man_made:pier.
  UNIT_TEST(CanadaVictoriaVancouverTest)
  {
    TRouteResult const routeResult =
        integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                    MercatorBounds::FromLatLon(48.47831, -123.32749), {0.0, 0.0},
                                    MercatorBounds::FromLatLon(49.26242, -123.11553));
    RouterResultCode const result = routeResult.second;
    TEST_EQUAL(result, RouterResultCode::NoError, ());
  }

  // Test on the route with the finish near zero length edge.
  UNIT_TEST(BelarusSlonimFinishNearZeroEdgeTest)
  {
    TRouteResult const routeResult =
        integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                    MercatorBounds::FromLatLon(53.08279, 25.30036), {0.0, 0.0},
                                    MercatorBounds::FromLatLon(53.09443, 25.34356));
    RouterResultCode const result = routeResult.second;
    TEST_EQUAL(result, RouterResultCode::NoError, ());
  }

  // Test on the route with the start near zero length edge.
  UNIT_TEST(BelarusSlonimStartNearZeroEdgeTest)
  {
    TRouteResult const routeResult =
        integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Car>(),
                                    MercatorBounds::FromLatLon(53.09422, 25.34411), {0.0, 0.0},
                                    MercatorBounds::FromLatLon(53.09271, 25.3467));
    RouterResultCode const result = routeResult.second;
    TEST_EQUAL(result, RouterResultCode::NoError, ());
  }
}  // namespace
