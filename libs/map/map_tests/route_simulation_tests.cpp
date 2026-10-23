#include "testing/testing.hpp"

#include "map/route_simulation.hpp"

#include "routing/route.hpp"

#include "platform/location.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point_with_altitude.hpp"

#include "base/math.hpp"

#include <chrono>
#include <cmath>
#include <optional>
#include <vector>

namespace route_simulation_tests
{
ms::LatLon const kStart(52.5, 13.4);

geometry::PointWithAltitude ToJunction(ms::LatLon const & latLon)
{
  return geometry::PointWithAltitude(mercator::FromLatLon(latLon), 0 /* altitude */);
}

geometry::PointWithAltitude NorthOfStart(double meters)
{
  return ToJunction({kStart.m_lat + math::RadToDeg(meters / ms::kEarthRadiusMeters), kStart.m_lon});
}

geometry::PointWithAltitude EastOfStart(double meters)
{
  double const parallelRadius = ms::kEarthRadiusMeters * std::cos(math::DegToRad(kStart.m_lat));
  return ToJunction({kStart.m_lat, kStart.m_lon + math::RadToDeg(meters / parallelRadius)});
}

std::vector<location::GpsInfo> Drain(RouteSimulation & simulation)
{
  size_t constexpr kMaxFixes = 100;
  std::vector<location::GpsInfo> fixes;
  for (auto fix = simulation.Next(0.0); fix && fixes.size() < kMaxFixes; fix = simulation.Next(0.0))
    fixes.push_back(*fix);
  return fixes;
}

UNIT_TEST(RouteSimulation_FirstFix)
{
  routing::RouteJunctions const junctions = {ToJunction(kStart), NorthOfStart(10.0)};
  RouteSimulation simulation(junctions);

  double const timestamp = 1700000000.5;
  auto const fix = simulation.Next(timestamp);
  TEST(fix, ("The first fix is missing."));
  auto const expected = junctions[0].ToLatLon();
  TEST_ALMOST_EQUAL_ABS(fix->m_latitude, expected.m_lat, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(fix->m_longitude, expected.m_lon, 1e-9, ());
  TEST_EQUAL(fix->m_horizontalAccuracy, 1.0, ());
  TEST_EQUAL(fix->m_source, location::EUser, ());
  TEST_EQUAL(fix->m_timestamp, timestamp, ());
  TEST(!fix->HasSpeed(), (fix->m_speed));
  TEST(!fix->HasBearing(), (fix->m_bearing));
}

UNIT_TEST(RouteSimulation_SpeedAndBearing)
{
  for (auto const & [finish, bearing] : {std::pair{NorthOfStart(10.0), 0.0}, std::pair{EastOfStart(10.0), 90.0}})
  {
    routing::RouteJunctions const junctions = {ToJunction(kStart), finish};
    RouteSimulation simulation(junctions);
    TEST(simulation.Next(0.0), ());

    auto const fix = simulation.Next(1.0);
    TEST(fix, ("The second fix is missing.", bearing));
    TEST_ALMOST_EQUAL_ABS(fix->m_speed, ms::DistanceOnEarth(junctions[0].ToLatLon(), junctions[1].ToLatLon()), 1e-6,
                          (bearing));
    TEST_ALMOST_EQUAL_ABS(fix->m_bearing, bearing, 1e-6, ());
  }
}

UNIT_TEST(RouteSimulation_IdenticalPointsKeepBearing)
{
  double const offset = mercator::MetersToMercator(10.0 / std::sqrt(2.0)) / std::cos(math::DegToRad(kStart.m_lat));
  auto const start = ToJunction(kStart);
  geometry::PointWithAltitude const finish(start.GetPoint() + m2::PointD(offset, offset), 0 /* altitude */);
  RouteSimulation simulation({start, finish, finish});
  TEST(simulation.Next(0.0), ());

  auto const second = simulation.Next(1.0);
  TEST(second, ());
  TEST_ALMOST_EQUAL_ABS(second->m_bearing, 45.0, 1e-3, ());

  auto const third = simulation.Next(2.0);
  TEST(third, ("The fix on the repeated point is missing."));
  TEST_EQUAL(third->m_speed, 0.0, ());
  TEST_ALMOST_EQUAL_ABS(third->m_bearing, 45.0, 1e-3, ());
}

UNIT_TEST(RouteSimulation_DensifiesAt50Kmh)
{
  // Just over two steps, so three segments are needed.
  double const lengthM = 2 * kRouteSimulationStepM + 0.05;
  routing::RouteJunctions const junctions = {ToJunction(kStart), NorthOfStart(lengthM)};
  TEST_ALMOST_EQUAL_ABS(ms::DistanceOnEarth(junctions[0].ToLatLon(), junctions[1].ToLatLon()), lengthM, 1e-6, ());
  RouteSimulation simulation(junctions);

  double const maxSpeed = kRouteSimulationStepM / std::chrono::duration<double>(kRouteSimulationInterval).count();
  auto const fixes = Drain(simulation);
  TEST_EQUAL(fixes.size(), 4, ());
  for (size_t i = 1; i < fixes.size(); ++i)
    TEST_LESS_OR_EQUAL(fixes[i].m_speed, maxSpeed + 1e-6, (i));
  auto const finish = junctions[1].ToLatLon();
  TEST_ALMOST_EQUAL_ABS(fixes.back().m_latitude, finish.m_lat, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(fixes.back().m_longitude, finish.m_lon, 1e-9, ());
}

UNIT_TEST(RouteSimulation_EndsAfterLastPoint)
{
  RouteSimulation simulation({ToJunction(kStart), NorthOfStart(10.0)});
  TEST(simulation.Next(0.0), ());
  TEST(simulation.Next(1.0), ());

  TEST(!simulation.Next(2.0), ("A drained simulation must not emit again."));
  TEST(!simulation.Next(3.0), ("A drained simulation must not emit again."));
}
}  // namespace route_simulation_tests
