#include "testing/testing.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "routing/route.hpp"
#include "routing/routing_callbacks.hpp"

#include "platform/location.hpp"

using namespace routing;
using namespace routing::turns;

void MoveRoute(Route & route, ms::LatLon const & coords)
{
  location::GpsInfo info;
  info.m_horizontalAccuracy = 0.01;
  info.m_verticalAccuracy = 0.01;
  info.m_longitude = coords.m_lon;
  info.m_latitude = coords.m_lat;
  route.MoveIterator(info);
}

UNIT_TEST(RussiaTulskayaToPaveletskayaStreetNamesTest)
{
  TRouteResult const routeResult = integration::CalculateRoute(integration::GetVehicleComponents(VehicleType::Car),
                                                               mercator::FromLatLon(55.70839, 37.62145), {0., 0.},
                                                               mercator::FromLatLon(55.73198, 37.63945));

  Route & route = *routeResult.first;
  RouterResultCode const result = routeResult.second;
  TEST_EQUAL(result, RouterResultCode::NoError, ());

  /// @todo https://github.com/organicmaps/organicmaps/issues/1668
  integration::TestCurrentStreetName(route, "Большая Тульская улица");
  integration::TestNextStreetName(route, "Подольское шоссе");

  MoveRoute(route, ms::LatLon(55.71398, 37.62443));

  integration::TestCurrentStreetName(route, "Подольское шоссе");
  integration::TestNextStreetName(route, "Валовая улица");

  MoveRoute(route, ms::LatLon(55.72059, 37.62766));

  integration::TestCurrentStreetName(route, "Павловская улица");
  integration::TestNextStreetName(route, "Валовая улица");

  MoveRoute(route, ms::LatLon(55.72469, 37.62624));

  integration::TestCurrentStreetName(route, "Большая Серпуховская улица");
  integration::TestNextStreetName(route, "Валовая улица");

  MoveRoute(route, ms::LatLon(55.73034, 37.63099));

  // No more extra last turn, so TestNextStreetName returns "".
  integration::TestCurrentStreetName(route, "Валовая улица");
  // integration::TestNextStreetName(route, "улица Зацепский Вал");

  MoveRoute(route, ms::LatLon(55.730912, 37.636191));

  integration::TestCurrentStreetName(route, "улица Зацепский Вал");
  integration::TestNextStreetName(route, "");

  integration::TestRouteLength(route, 3390.);
}
