#include "testing/testing.hpp"

#include "generator/cities_boundaries_checker.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"

#include "base/logging.hpp"

namespace
{
using namespace generator;
using namespace indexer;

UNIT_TEST(CitiesBoundariesChecker_Square)
{
  auto const checker = CitiesBoundariesChecker({CityBoundary({{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}})});

  TEST(checker.InCity({0.5, 0.5}), ());
  TEST(checker.InCity({0.0001, 0.0001}), ());

  TEST(!checker.InCity({2.0, 2.0}), ());
  TEST(!checker.InCity({20.0, 20.0}), ());
  TEST(!checker.InCity({-30.0, 30.0}), ());
  TEST(!checker.InCity({40.0, -40.0}), ());
}

UNIT_TEST(CitiesBoundariesChecker_NotConvexPolygon)
{
  auto const checker =
      CitiesBoundariesChecker({CityBoundary({{0.0, 0.0}, {1.0, -1.0}, {0.5, 0.0}, {1.0, 1.0}, {0.0, 1.0}})});

  TEST(checker.InCity({0.3, 0.3}), ());
  TEST(checker.InCity({0.0001, 0.0001}), ());

  TEST(!checker.InCity({2.0, 2.0}), ());
  TEST(!checker.InCity({20.0, 20.0}), ());
  TEST(!checker.InCity({-30.0, 30.0}), ());
  TEST(!checker.InCity({40.0, -40.0}), ());
}

UNIT_TEST(CitiesBoundariesChecker_IntersectedPolygons)
{
  auto const checker =
      CitiesBoundariesChecker({CityBoundary({{0.0, 0.0}, {1.0, -1.0}, {0.5, 0.0}, {1.0, 1.0}, {0.0, 1.0}}),
                               CityBoundary({{0.0, 0.0}, {1.0, -1.0}, {0.5, 0.0}, {1.0, 1.0}, {0.0, 1.0}})});

  TEST(checker.InCity({0.3, 0.3}), ());
  TEST(checker.InCity({0.0001, 0.0001}), ());

  TEST(!checker.InCity({2.0, 2.0}), ());
  TEST(!checker.InCity({20.0, 20.0}), ());
  TEST(!checker.InCity({-30.0, 30.0}), ());
  TEST(!checker.InCity({40.0, -40.0}), ());
}

UNIT_TEST(CitiesBoundariesChecker_SeveralPolygons)
{
  auto const checker =
      CitiesBoundariesChecker({CityBoundary({{0.0, 0.0}, {1.0, -1.0}, {0.5, 0.0}, {1.0, 1.0}, {0.0, 1.0}}),
                               CityBoundary({{10.0, 0.0}, {11.0, -1.0}, {10.5, 0.0}, {11.0, 1.0}, {10.0, 1.0}}),
                               CityBoundary({{0.0, 10.0}, {1.0, -11.0}, {0.5, 10.0}, {1.0, 11.0}, {0.0, 11.0}})});

  TEST(checker.InCity({0.3, 0.3}), ());
  TEST(checker.InCity({0.0001, 0.0001}), ());

  TEST(checker.InCity({10.3, 0.3}), ());
  TEST(checker.InCity({10.0001, 0.0001}), ());

  TEST(checker.InCity({0.3, 10.4}), ());
  TEST(checker.InCity({0.0001, 10.0001}), ());

  TEST(!checker.InCity({2.0, 2.0}), ());
  TEST(!checker.InCity({20.0, 20.0}), ());
  TEST(!checker.InCity({-30.0, 30.0}), ());
  TEST(!checker.InCity({40.0, -40.0}), ());
}
}  // namespace
