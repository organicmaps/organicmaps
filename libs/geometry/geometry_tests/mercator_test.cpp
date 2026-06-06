#include "testing/testing.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"

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

UNIT_TEST(Mercator_WrapX)
{
  TEST_ALMOST_EQUAL_ABS(mercator::WrapX(185.0), -175.0, 1e-10, ());
  TEST_ALMOST_EQUAL_ABS(mercator::WrapX(-185.0), 175.0, 1e-10, ());
  TEST_ALMOST_EQUAL_ABS(mercator::WrapX(0.0), 0.0, 1e-10, ());
  TEST_ALMOST_EQUAL_ABS(mercator::WrapX(-180.0), -180.0, 1e-10, ());
  TEST_ALMOST_EQUAL_ABS(mercator::WrapX(180.0), -180.0, 1e-10, ());
  TEST_ALMOST_EQUAL_ABS(mercator::WrapX(540.0), -180.0, 1e-10, ());
  TEST_ALMOST_EQUAL_ABS(mercator::WrapX(-540.0), -180.0, 1e-10, ());
}

UNIT_TEST(Mercator_NearestWrapX)
{
  // No adjustment needed (within 180 of reference).
  TEST_ALMOST_EQUAL_ABS(mercator::NearestWrapX(10.0, 0.0), 10.0, 1e-10, ());
  TEST_ALMOST_EQUAL_ABS(mercator::NearestWrapX(-170.0, -170.0), -170.0, 1e-10, ());

  // Wrap westward: point is > 180 east of reference.
  TEST_ALMOST_EQUAL_ABS(mercator::NearestWrapX(170.0, -20.0), -190.0, 1e-10, ());

  // Wrap eastward: point is > 180 west of reference.
  TEST_ALMOST_EQUAL_ABS(mercator::NearestWrapX(-170.0, 20.0), 190.0, 1e-10, ());

  // Exactly at 180 boundary — no adjustment (strict inequality).
  TEST_ALMOST_EQUAL_ABS(mercator::NearestWrapX(180.0, 0.0), 180.0, 1e-10, ());
  TEST_ALMOST_EQUAL_ABS(mercator::NearestWrapX(-180.0, 0.0), -180.0, 1e-10, ());

  // Extended screen origin (past antimeridian, single wrap).
  TEST_ALMOST_EQUAL_ABS(mercator::NearestWrapX(-175.0, 350.0), 185.0, 1e-10, ());

  // Extended screen origin requiring two wraps.
  TEST_ALMOST_EQUAL_ABS(mercator::NearestWrapX(-170.0, 500.0), 550.0, 1e-10, ());
  TEST_ALMOST_EQUAL_ABS(mercator::NearestWrapX(170.0, -500.0), -550.0, 1e-10, ());

  // Edge case: max normalization range boundary.
  TEST_ALMOST_EQUAL_ABS(mercator::NearestWrapX(-180.0, 540.0), 540.0, 1e-10, ());
  TEST_ALMOST_EQUAL_ABS(mercator::NearestWrapX(180.0, -540.0), -540.0, 1e-10, ());
}

UNIT_TEST(Mercator_NearestWrapX_NeverHangs)
{
  using mercator::NearestWrapX;
  using L = std::numeric_limits<double>;

  // Huge finite refX: |x - refX| > 2^53, where x +/- 360 == x in double
  // precision. The earlier while-loop implementation infinite-loops here.
  // The exact returned value is not meaningful at this scale (precision is
  // lost); the only requirement is that the call returns in bounded time
  // and produces a finite value.
  TEST(std::isfinite(NearestWrapX(0.0, 1e18)), ());
  TEST(std::isfinite(NearestWrapX(1e18, 0.0)), ());
  TEST(std::isfinite(NearestWrapX(0.0, -1e18)), ());
  TEST(std::isfinite(NearestWrapX(-1e18, 0.0)), ());

  // Infinity: must return promptly. Inf inputs are propagated rather than
  // forced into [-180, 180] (they cannot be), but the function must not hang.
  TEST_EQUAL(NearestWrapX(L::infinity(), 0.0), L::infinity(), ());
  TEST_EQUAL(NearestWrapX(-L::infinity(), 0.0), -L::infinity(), ());
  TEST_EQUAL(NearestWrapX(0.0, L::infinity()), 0.0, ());
  TEST_EQUAL(NearestWrapX(0.0, -L::infinity()), 0.0, ());

  // NaN: must return promptly. NaN propagates per IEEE 754.
  TEST(std::isnan(NearestWrapX(L::quiet_NaN(), 0.0)), ());
  TEST_EQUAL(NearestWrapX(0.0, L::quiet_NaN()), 0.0, ());

  // Many wraps. The original "max 2 iterations" assumption only held
  // when NormalizeScreenOriginX kept the origin in [-540, 540], but that
  // function is currently disabled. Verify correctness for arbitrary
  // multi-wrap distances: the result must be within 180 of refX, and the
  // adjustment must be an integer multiple of 360 (i.e. same world copy
  // as the original x).
  for (int worlds = 0; worlds <= 100; ++worlds)
  {
    double const refX = -170.0 + worlds * 360.0;
    double const result = NearestWrapX(170.0, refX);
    TEST_LESS_OR_EQUAL(std::abs(result - refX), 180.0, (worlds, refX, result));
    double const adjustment = result - 170.0;
    double const remainder = adjustment - std::round(adjustment / 360.0) * 360.0;
    TEST_LESS(std::abs(remainder), 1e-9, (worlds, refX, result, adjustment, remainder));
  }
}
