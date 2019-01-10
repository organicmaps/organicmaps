#include "testing/testing.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "geometry/mercator.hpp"

using namespace routing;

namespace
{
UNIT_TEST(Moscow_CenterToKotelniki_CrossMwm)
{
  TRouteResult routeResult =
    integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Transit>(),
                                MercatorBounds::FromLatLon(55.75018, 37.60971), {0., 0.},
                                MercatorBounds::FromLatLon(55.67245, 37.86130));
  TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());

  integration::TestRouteLength(*routeResult.first, 22968.6);

  CHECK(routeResult.first, ());
  integration::CheckSubwayExistence(*routeResult.first);
}

UNIT_TEST(Moscow_DubrovkaToTrtykovskya)
{
  TRouteResult routeResult =
    integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Transit>(),
                                MercatorBounds::FromLatLon(55.71813, 37.67756), {0., 0.},
                                MercatorBounds::FromLatLon(55.74089, 37.62831));
  TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());

  integration::TestRouteLength(*routeResult.first, 7622.19);

  CHECK(routeResult.first, ());
  integration::CheckSubwayExistence(*routeResult.first);
}

UNIT_TEST(Moscow_NoSubwayTest)
{
  TRouteResult routeResult =
    integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Transit>(),
                                MercatorBounds::FromLatLon(55.73893, 37.62438), {0., 0.},
                                MercatorBounds::FromLatLon(55.73470, 37.62617));
  TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());

  integration::TestRouteLength(*routeResult.first, 604.847);

  CHECK(routeResult.first, ());
  integration::CheckSubwayAbsent(*routeResult.first);
}

UNIT_TEST(Piter_FrunzenskyaToPlochadVosstaniya)
{
  TRouteResult routeResult =
    integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Transit>(),
                                MercatorBounds::FromLatLon(59.90511, 30.31425), {0., 0.},
                                MercatorBounds::FromLatLon(59.93096, 30.35872));
  TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());

  integration::TestRouteLength(*routeResult.first, 4952.43);

  CHECK(routeResult.first, ());
  integration::CheckSubwayExistence(*routeResult.first);
}

UNIT_TEST(Piter_TooLongPedestrian)
{
  TRouteResult routeResult =
    integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Transit>(),
                                MercatorBounds::FromLatLon(59.90511, 30.31425), {0., 0.},
                                MercatorBounds::FromLatLon(59.80677, 30.44749));

  TEST_EQUAL(routeResult.second, RouterResultCode::TransitRouteNotFoundTooLongPedestrian, ());
}

UNIT_TEST(Vatikan_NotEnoughGraphDataAtThenEnd)
{
  TRouteResult routeResult =
    integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Transit>(),
                                MercatorBounds::FromLatLon(41.90052, 12.42642), {0., 0.},
                                MercatorBounds::FromLatLon(41.90253, 12.45574));

  // TODO (@gmoryes) here must be RouteNotFound.
  TEST_EQUAL(routeResult.second, RouterResultCode::TransitRouteNotFoundTooLongPedestrian, ());
}

UNIT_TEST(Vatikan_CorneliaToOttaviano)
{
  TRouteResult routeResult =
    integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Transit>(),
                                MercatorBounds::FromLatLon(41.90052, 12.42642), {0., 0.},
                                MercatorBounds::FromLatLon(41.90414, 12.45640));

  TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());

  integration::TestRouteLength(*routeResult.first, 4255.16);

  CHECK(routeResult.first, ());
  integration::CheckSubwayExistence(*routeResult.first);
}

UNIT_TEST(London_PoplarToOval)
{
  TRouteResult routeResult =
    integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Transit>(),
                                MercatorBounds::FromLatLon(51.50818, -0.01634), {0., 0.},
                                MercatorBounds::FromLatLon(51.48041, -0.10843));

  TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());

  integration::TestRouteLength(*routeResult.first, 9421.72);

  CHECK(routeResult.first, ());
  integration::CheckSubwayExistence(*routeResult.first);
}

UNIT_TEST(London_DeptfordBridgeToCyprus)
{
  TRouteResult routeResult =
    integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Transit>(),
                                MercatorBounds::FromLatLon(51.47149, -0.030558), {0., 0.},
                                MercatorBounds::FromLatLon(51.51242, 0.07101));

  TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());

  integration::TestRouteLength(*routeResult.first, 12293.4);

  CHECK(routeResult.first, ());
  integration::CheckSubwayExistence(*routeResult.first);
}

UNIT_TEST(Vashington_FoggyToShaw)
{
  TRouteResult routeResult =
    integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Transit>(),
                                MercatorBounds::FromLatLon(38.89582, -77.04934), {0., 0.},
                                MercatorBounds::FromLatLon(38.91516, -77.01513));

  TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());

  integration::TestRouteLength(*routeResult.first, 6102.92);

  CHECK(routeResult.first, ());
  integration::CheckSubwayExistence(*routeResult.first);
}

UNIT_TEST(NewYork_GrassmereToPleasantPlains)
{
  TRouteResult routeResult =
    integration::CalculateRoute(integration::GetVehicleComponents<VehicleType::Transit>(),
                                MercatorBounds::FromLatLon(40.60536, -74.07736), {0., 0.},
                                MercatorBounds::FromLatLon(40.53015, -74.21559));

  TEST_EQUAL(routeResult.second, RouterResultCode::NoError, ());

  integration::TestRouteLength(*routeResult.first, 17409.7);

  CHECK(routeResult.first, ());
  integration::CheckSubwayExistence(*routeResult.first);
}
}  // namespace
