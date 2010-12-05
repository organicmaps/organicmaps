#include "../../base/SRC_FIRST.hpp"

#include "../../testing/testing.hpp"
#include "../mercator.hpp"
#include "../../base/math.hpp"

UNIT_TEST(MercatorTestGrid)
{
  double const eps = 0.0000001;
  for (int lat = -85; lat <= 85; ++lat)
  {
    for (int lon = -180; lon <= 180; ++lon)
    {
      double const x = MercatorBounds::LonToX(lon);
      double const y = MercatorBounds::LatToY(lat);
      double const lat1 = MercatorBounds::YToLat(y);
      double const lon1 = MercatorBounds::XToLon(x);

      // Normal assumption for any projection.
      TEST_ALMOST_EQUAL(static_cast<double>(lat), lat1, ());
      TEST_ALMOST_EQUAL(static_cast<double>(lon), lon1, ());

      // x is actually lon unmodified.
      TEST_ALMOST_EQUAL(x, static_cast<double>(lon), ());

      if (lat == 0)
      {
        // TODO: Investigate, how to make Mercator transform more precise.
        // Error is to large for TEST_ALMOST_EQUAL(y, 0.0, ());
        TEST_LESS(fabs(y), eps, (lat, y, lat1));
      }
    }
  }
}

UNIT_TEST(MercatorTest)
{
  double const eps = 0.0000001;
  double lon = 63.45421;
  double x = MercatorBounds::LonToX(lon);
  double lon1 = MercatorBounds::XToLon(x);
  TEST_LESS(fabs(lon - lon1), eps, ("Too big round error"));
  double lat = 34.28754;
  double y = MercatorBounds::LatToY(lat);
  double lat1 = MercatorBounds::YToLat(y);
  TEST_LESS(fabs(lat - lat1), eps, ("Too big round error"));
  TEST_LESS(fabs(MercatorBounds::maxX - MercatorBounds::maxY), eps, ("Non-square maxX and maxY"));
  TEST_LESS(fabs(MercatorBounds::minX - MercatorBounds::minY), eps, ("Non-square minX and minY"));
}
