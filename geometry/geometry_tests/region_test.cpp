#include "../../testing/testing.hpp"

#include "../../base/macros.hpp"

#include "../region2d.hpp"

template<class TPoint>
void Test()
{
  m2::Region<TPoint> region;

  typedef TPoint P;

  // rectangular polygon
  {
    P const data[] = { P(1, 1), P(10, 1), P(10, 10), P(1, 10) };
    region.Assign(data, data + ARRAY_SIZE(data));
  }
  TEST_EQUAL(region.Rect(), m2::Rect<typename TPoint::value_type>(1, 1, 10, 10), ());

  TEST(region.Contains(P(1, 1)), ());
  TEST(region.Contains(P(2, 2)), ());
  TEST(region.Contains(P(10, 5)), ());
  TEST(region.Contains(P(1, 6)), ());
  TEST(!region.Contains(P(0, 0)), ());
  TEST(!region.Contains(P(100, 0)), ());

  // triangle
  {
    P const data[] = {P(1, 1), P(10, 1), P(10, 10), P(1, 10)};
    region.Assign(data, data + ARRAY_SIZE(data));
  }

}

UNIT_TEST(Region_Contains)
{
  Test<m2::PointU>();
  Test<m2::PointD>();
  Test<m2::PointF>();
  Test<m2::PointI>();

  // negative triangle
  {
    typedef m2::PointI P;
    m2::Region<P> region;
    P const data[] = { P(1, -1), P(-2, -2), P(-3, 1) };
    region.Assign(data, data + ARRAY_SIZE(data));

    TEST_EQUAL(region.Rect(), m2::Rect<P::value_type>(-3, -2, 1, 1), ());

    TEST(region.Contains(P(-2, -2)), ());
    TEST(region.Contains(P(-2, 0)), ());
    TEST(!region.Contains(P(0, 0)), ());
  }

  {
    typedef m2::PointI P;
    m2::Region<P> region;
    P const data[] = { P(1, -1), P(3, 0), P(3, 3), P(0, 3), P(0, 2), P(0, 1), P(2, 2) };
    region.Assign(data, data + ARRAY_SIZE(data));

    TEST_EQUAL(region.Rect(), m2::Rect<P::value_type>(0, -1, 3, 3), ());

    TEST(region.Contains(P(2, 2)), ());
    TEST(region.Contains(P(1, 3)), ());
    TEST(region.Contains(P(3, 1)), ());
    TEST(!region.Contains(P(1, 1)), ());
  }
}
