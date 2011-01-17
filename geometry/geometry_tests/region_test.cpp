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

UNIT_TEST(Region)
{
  typedef m2::PointD P;
  P p1[] = { P(0.1, 0.2) };

  m2::Region<P> region(p1, p1 + ARRAY_SIZE(p1));
  TEST(!region.IsValid(), ());

  {
    P p2[] = { P(1.0, 2.0), P(55.0, 33.0) };
    region.Assign(p2, p2 + ARRAY_SIZE(p2));
  }
  TEST(!region.IsValid(), ());

  region.AddPoint(P(34.4, 33.2));
  TEST(region.IsValid(), ());

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

template <class TPoint>
struct PointsSummator
{
  double m_xSumm;
  double m_ySumm;
  PointsSummator() : m_xSumm(0), m_ySumm(0) {}
  void operator()(TPoint const & pt)
  {
    m_xSumm += pt.x;
    m_ySumm += pt.y;
  }
};

UNIT_TEST(Region_ForEachPoint)
{
  typedef m2::PointF P;
  P const points[] = { P(0.0, 1.0), P(1.0, 2.0), P(10.5, 11.5) };
  m2::Region<P> region(points, points + ARRAY_SIZE(points));

  PointsSummator<P> s;
  region.ForEachPoint(s);

  TEST_EQUAL(s.m_xSumm, 11.5, ());
  TEST_EQUAL(s.m_ySumm, 14.5, ());
}
