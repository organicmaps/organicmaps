#include "testing/testing.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/math.hpp"

UNIT_TEST(Mercator_Grid)
{
  for (int lat = -85; lat <= 85; ++lat)
  {
    for (int lon = -180; lon <= 180; ++lon)
    {
      double const x = mercator::LonToX(lon);
      double const y = mercator::LatToY(lat);
      double const lat1 = mercator::YToLat(y);
      double const lon1 = mercator::XToLon(x);

      // Normal assumption for any projection.
      TEST_ALMOST_EQUAL_ULPS(static_cast<double>(lat), lat1, ());
      TEST_ALMOST_EQUAL_ULPS(static_cast<double>(lon), lon1, ());

      // x is actually lon unmodified.
      TEST_ALMOST_EQUAL_ULPS(x, static_cast<double>(lon), ());
    }
  }
}

UNIT_TEST(Mercator_DirectInferseF)
{
  double const eps = 0.0000001;
  double lon = 63.45421;
  double x = mercator::LonToX(lon);
  double lon1 = mercator::XToLon(x);
  TEST_LESS(fabs(lon - lon1), eps, ("Too big round error"));
  double lat = 34.28754;
  double y = mercator::LatToY(lat);
  double lat1 = mercator::YToLat(y);
  TEST_LESS(fabs(lat - lat1), eps, ("Too big round error"));
  TEST_LESS(fabs(mercator::Bounds::kMaxX - mercator::Bounds::kMaxY), eps, ("Non-square maxX and maxY"));
  TEST_LESS(fabs(mercator::Bounds::kMinX - mercator::Bounds::kMinY), eps, ("Non-square minX and minY"));
}

UNIT_TEST(Mercator_ErrorToRadius)
{
  double const points[] = {-85.0, -45.0, -10.0, -1.0, -0.003, 0.0, 0.003, 1.0, 10.0, 45.0, 85.0};

  double const error1 = 1.0;    // 1 metre
  double const error10 = 10.0;  // 10 metres

  for (size_t i = 0; i < ARRAY_SIZE(points); ++i)
  {
    for (size_t j = 0; j < ARRAY_SIZE(points); ++j)
    {
      double const lon = points[i];
      double const lat = points[j];
      m2::PointD const mercPoint(mercator::LonToX(lon), mercator::LatToY(lat));

      m2::RectD const radius1 = mercator::MetersToXY(lon, lat, error1);
      TEST(radius1.IsPointInside(mercPoint), (lat, lon));
      TEST(radius1.Center().EqualDxDy(mercPoint, 1.0E-8), ());

      m2::RectD const radius10 = mercator::MetersToXY(lon, lat, error10);
      TEST(radius10.IsPointInside(mercPoint), (lat, lon));
      TEST(radius10.Center().EqualDxDy(mercPoint, 1.0E-8), ());

      TEST_EQUAL(m2::Add(radius10, radius1), radius10, (lat, lon));

      TEST(radius10.IsPointInside(radius1.LeftTop()), (lat, lon));
      TEST(radius10.IsPointInside(radius1.LeftBottom()), (lat, lon));
      TEST(radius10.IsPointInside(radius1.RightTop()), (lat, lon));
      TEST(radius10.IsPointInside(radius1.RightBottom()), (lat, lon));
    }
  }
}

UNIT_TEST(Mercator_Sample1)
{
  LOG(LINFO, (mercator::XToLon(27.531491200000001385), mercator::YToLat(64.392864299248202542)));
}
