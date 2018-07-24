#include "testing/testing.hpp"

#include "base/macros.hpp"

#include "geometry/region2d.hpp"

namespace
{
template <class Region>
struct ContainsChecker
{
  ContainsChecker(Region const & region) : m_region(region) {}

  void operator()(typename Region::Value const & pt)
  {
    TEST(m_region.Contains(pt), ("Region should contain all its points"));
  }

  Region const & m_region;
};

/// Region should have CCW orientation from left, down corner.
template <class Point>
void TestContainsRectangular(Point const * arr)
{
  m2::Region<Point> region;

  size_t const count = 4;
  region.Assign(arr, arr + count);

  for (size_t i = 0; i < count; ++i)
  {
    region.Contains(arr[i]);
    region.Contains((arr[i] + arr[(i + count - 1) % count]) / 2);
  }

  Point dx(1, 0);
  Point dy(0, 1);

  TEST(!region.Contains(arr[0] - dx), ());
  TEST(!region.Contains(arr[0] - dy), ());
  TEST(!region.Contains(arr[0] - dx - dy), ());
  TEST(region.Contains(arr[0] + dx + dy), ());

  TEST(!region.Contains(arr[1] + dx), ());
  TEST(!region.Contains(arr[1] - dy), ());
  TEST(!region.Contains(arr[1] + dx - dy), ());
  TEST(region.Contains(arr[1] - dx + dy), ());

  TEST(!region.Contains(arr[2] + dx), ());
  TEST(!region.Contains(arr[2] + dy), ());
  TEST(!region.Contains(arr[2] + dx + dy), ());
  TEST(region.Contains(arr[2] - dx - dy), ());

  TEST(!region.Contains(arr[3] - dx), ());
  TEST(!region.Contains(arr[3] + dy), ());
  TEST(!region.Contains(arr[3] - dx + dy), ());
  TEST(region.Contains(arr[3] + dx - dy), ());
}

template <class Region>
void TestContains()
{
  Region region;
  ContainsChecker<Region> checker(region);

  // point type
  using P = typename Region::Value;

  // rectangular polygon
  {
    P const data[] = {P(1, 1), P(10, 1), P(10, 10), P(1, 10)};
    TestContainsRectangular(data);
  }
  {
    P const data[] = {P(-100, -100), P(-50, -100), P(-50, -50), P(-100, -50)};
    TestContainsRectangular(data);
  }
  {
    P const data[] = {P(-2000000000, -2000000000), P(-1000000000, -2000000000),
                      P(-1000000000, -1000000000), P(-2000000000, -1000000000)};
    TestContainsRectangular(data);
  }
  {
    P const data[] = {P(1000000000, 1000000000), P(2000000000, 1000000000),
                      P(2000000000, 2000000000), P(1000000000, 2000000000)};
    TestContainsRectangular(data);
  }

  // triangle
  {
    P const data[] = {P(0, 0), P(2, 0), P(2, 2)};
    region.Assign(data, data + ARRAY_SIZE(data));
  }
  TEST_EQUAL(region.GetRect(), m2::Rect<typename P::value_type>(0, 0, 2, 2), ());
  TEST(region.Contains(P(2, 0)), ());
  TEST(region.Contains(P(1, 1)), ("point on diagonal"));
  TEST(!region.Contains(P(33, 0)), ());
  region.ForEachPoint(checker);

  // complex polygon
  {
    P const data[] = {P(0, 0), P(2, 0), P(2, 2), P(3, 1), P(4, 2), P(5, 2), P(3, 3),
                      P(3, 2), P(2, 4), P(6, 3), P(7, 4), P(7, 2), P(8, 5), P(8, 7),
                      P(7, 7), P(8, 8), P(5, 9), P(6, 6), P(5, 7), P(4, 6), P(4, 8),
                      P(3, 7), P(2, 7), P(3, 6), P(4, 4), P(0, 7), P(2, 3), P(0, 2)};
    region.Assign(data, data + ARRAY_SIZE(data));
  }
  TEST_EQUAL(region.GetRect(), m2::Rect<typename P::value_type>(0, 0, 8, 9), ());
  TEST(region.Contains(P(0, 0)), ());
  TEST(region.Contains(P(3, 7)), ());
  TEST(region.Contains(P(1, 2)), ());
  TEST(region.Contains(P(1, 1)), ());
  TEST(!region.Contains(P(6, 2)), ());
  TEST(!region.Contains(P(3, 5)), ());
  TEST(!region.Contains(P(5, 8)), ());
  region.ForEachPoint(checker);
}

template <class Point>
class PointsSummator
{
public:
  PointsSummator(Point & res) : m_res(res) {}

  void operator()(Point const & pt) { m_res += pt; }

private:
  Point & m_res;
};
}  // namespace

UNIT_TEST(Region)
{
  typedef m2::PointD P;
  P p1[] = {P(0.1, 0.2)};

  m2::Region<P> region(p1, p1 + ARRAY_SIZE(p1));
  TEST(!region.IsValid(), ());

  {
    P p2[] = {P(1.0, 2.0), P(55.0, 33.0)};
    region.Assign(p2, p2 + ARRAY_SIZE(p2));
  }
  TEST(!region.IsValid(), ());

  region.AddPoint(P(34.4, 33.2));
  TEST(region.IsValid(), ());

  {
    // equality case
    {
      P const data[] = {P(1, 1),    P(0, 4.995), P(1, 4.999996), P(1.000003, 5.000001),
                        P(0.5, 10), P(10, 10),   P(10, 1)};
      region.Assign(data, data + ARRAY_SIZE(data));
    }
    TEST(!region.Contains(P(0.9999987, 0.9999938)), ());
    TEST(!region.Contains(P(0.999998, 4.9999987)), ());
  }
}

UNIT_TEST(Region_Contains_int32)
{
  TestContains<m2::RegionI>();

  // negative triangle
  {
    using P = m2::PointI;
    m2::Region<P> region;
    P const data[] = {P(1, -1), P(-2, -2), P(-3, 1)};
    region.Assign(data, data + ARRAY_SIZE(data));

    TEST_EQUAL(region.GetRect(), m2::Rect<P::value_type>(-3, -2, 1, 1), ());

    TEST(region.Contains(P(-2, -2)), ());
    TEST(region.Contains(P(-2, 0)), ());
    TEST(!region.Contains(P(0, 0)), ());
  }

  {
    using P = m2::PointI;
    m2::Region<P> region;
    P const data[] = {P(1, -1), P(3, 0), P(3, 3), P(0, 3), P(0, 2), P(0, 1), P(2, 2)};
    region.Assign(data, data + ARRAY_SIZE(data));

    TEST_EQUAL(region.GetRect(), m2::Rect<P::value_type>(0, -1, 3, 3), ());

    TEST(region.Contains(P(2, 2)), ());
    TEST(region.Contains(P(1, 3)), ());
    TEST(region.Contains(P(3, 1)), ());
    TEST(!region.Contains(P(1, 1)), ());
  }
}

UNIT_TEST(Region_Contains_uint32) { TestContains<m2::RegionU>(); }

UNIT_TEST(Region_Contains_double)
{
  using Region = m2::RegionD;
  using Point = Region::Value;

  TestContains<Region>();

  {
    Region region;
    Point const data[] = {{0, 7}, {4, 4}, {3, 6}, {8, 6}, {8, 5}, {6, 3}, {2, 2}};
    region.Assign(data, data + ARRAY_SIZE(data));

    TEST_EQUAL(region.GetRect(), m2::Rect<Point::value_type>(0, 2, 8, 7), ());
    TEST(!region.Contains({3, 5}), ());
  }
}

UNIT_TEST(Region_ForEachPoint)
{
  typedef m2::PointF P;
  P const points[] = {P(0.0, 1.0), P(1.0, 2.0), P(10.5, 11.5)};
  m2::Region<P> region(points, points + ARRAY_SIZE(points));

  P res(0, 0);
  region.ForEachPoint(PointsSummator<P>(res));

  TEST_EQUAL(res, P(11.5, 14.5), ());
}

UNIT_TEST(Region_point_at_border_test)
{
  typedef m2::PointF P;
  P const points[] = {P(0.0, 1.0), P(0.0, 10.0), P(10.0, 10.0), P(10.0, 1.0)};
  m2::Region<P> region(points, points + ARRAY_SIZE(points));

  P p1(0, 0);
  P p2(5.0, 5.0);
  P p3(0.0, 1.0);
  P p4(5.0, 1.0);
  P p5(5.0, 1.01);

  TEST(!region.AtBorder(p1, 0.01), ("Point lies outside the border"));
  TEST(!region.AtBorder(p2, 0.01), ("Point lies strictly inside the border"));
  TEST(region.AtBorder(p3, 0.01), ("Point has same point with the border"));
  TEST(region.AtBorder(p4, 0.01), ("Point lies at the border"));
  TEST(region.AtBorder(p5, 0.01), ("Point lies at delta interval near the border"));
}

UNIT_TEST(Region_border_intersecion_Test)
{
  typedef m2::PointF P;
  P const points[] = {P(0.0, 1.0), P(0.0, 10.0), P(10.0, 10.0), P(10.0, 1.0)};
  m2::Region<P> region(points, points + ARRAY_SIZE(points));

  P intersection;

  TEST(region.FindIntersection(P(5.0, 5.0), P(15.0, 5.0), intersection), ());
  TEST(intersection == P(10.0, 5.0), ());

  TEST(region.FindIntersection(P(5.0, 5.0), P(15.0, 15.0), intersection), ());
  TEST(intersection == P(10.0, 10.0), ());

  TEST(region.FindIntersection(P(7.0, 7.0), P(7.0, 10.0), intersection), ());
  TEST(intersection == P(7.0, 10.0), ());

  TEST(!region.FindIntersection(P(5.0, 5.0), P(2.0, 2.0), intersection),
       ("This case has no intersection"));
}
