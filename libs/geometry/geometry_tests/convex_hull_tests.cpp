#include "testing/testing.hpp"

#include "geometry/convex_hull.hpp"
#include "geometry/point2d.hpp"

#include <vector>

namespace convex_hull_tests
{
using namespace m2;
using namespace std;

double constexpr kEps = 1e-12;

vector<PointD> BuildConvexHull(vector<PointD> const & points)
{
  return ConvexHull(points, kEps).Points();
}

UNIT_TEST(ConvexHull_Smoke)
{
  TEST_EQUAL(BuildConvexHull({}), vector<PointD>{}, ());
  TEST_EQUAL(BuildConvexHull({PointD(0, 0)}), vector<PointD>{PointD(0, 0)}, ());
  TEST_EQUAL(BuildConvexHull({PointD(0, 0), PointD(0, 0)}), vector<PointD>{PointD(0, 0)}, ());

  TEST_EQUAL(BuildConvexHull({PointD(0, 0), PointD(1, 1), PointD(0, 0)}), vector<PointD>({PointD(0, 0), PointD(1, 1)}),
             ());

  TEST_EQUAL(BuildConvexHull({PointD(0, 0), PointD(1, 1), PointD(2, 2)}), vector<PointD>({PointD(0, 0), PointD(2, 2)}),
             ());

  {
    int const kXMax = 100;
    int const kYMax = 200;
    vector<PointD> points;
    for (int x = 0; x <= kXMax; ++x)
      for (int y = 0; y <= kYMax; ++y)
        points.emplace_back(x, y);
    TEST_EQUAL(BuildConvexHull(points),
               vector<PointD>({PointD(0, 0), PointD(kXMax, 0), PointD(kXMax, kYMax), PointD(0, kYMax)}), ());
  }

  TEST_EQUAL(BuildConvexHull({PointD(0, 0), PointD(0, 5), PointD(10, 5), PointD(3, 3), PointD(10, 0)}),
             vector<PointD>({PointD(0, 0), PointD(10, 0), PointD(10, 5), PointD(0, 5)}), ());
}
}  // namespace convex_hull_tests
