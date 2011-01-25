#include "../../testing/testing.hpp"

#include "../../base/macros.hpp"

#include "../region2d.hpp"

template<class RegionT> struct ContainsChecker
{
  RegionT & m_region;
  ContainsChecker(RegionT & region) : m_region(region) {}
  void operator()(typename RegionT::value_type const & pt)
  {
    TEST(m_region.Contains(pt), ("Region should contain all it's points"));
  }
};

template<class RegionT>
void Test()
{
  RegionT region;

  // point type
  typedef typename RegionT::value_type P;

  // rectangular polygon
  {
    P const data[] = { P(1, 1), P(10, 1), P(10, 10), P(1, 10) };
    region.Assign(data, data + ARRAY_SIZE(data));
  }
  TEST_EQUAL(region.Rect(), m2::Rect<typename P::value_type>(1, 1, 10, 10), ());

  TEST(region.Contains(P(1, 1)), ());
  TEST(region.Contains(P(2, 2)), ());
  TEST(region.Contains(P(10, 5)), ());
  TEST(region.Contains(P(1, 6)), ());
  TEST(!region.Contains(P(0, 0)), ());
  TEST(!region.Contains(P(100, 0)), ());
  ContainsChecker<RegionT> checker(region);
  region.ForEachPoint(checker);

  // triangle
  {
    P const data[] = {P(0, 0), P(2, 0), P(2, 2) };
    region.Assign(data, data + ARRAY_SIZE(data));
  }
  TEST_EQUAL(region.Rect(), m2::Rect<typename P::value_type>(0, 0, 2, 2), ());
  TEST(region.Contains(P(2, 0)), ());
  TEST(region.Contains(P(1, 1)), ("point on diagonal"));
  TEST(!region.Contains(P(33, 0)), ());
  region.ForEachPoint(checker);

  // complex polygon
  {
    P const data[] = { P(0, 0), P(2, 0), P(2, 2), P(3, 1), P(4, 2), P(5, 2),
        P(3, 3), P(3, 2), P(2, 4), P(6, 3), P(7, 4), P(7, 2), P(8, 5), P(8, 7),
        P(7, 7), P(8, 8), P(5, 9), P(6, 6), P(5, 7), P(4, 6), P(4, 8), P(3, 7),
        P(2, 7), P(3, 6), P(4, 4), P(0, 7), P(2, 3), P(0, 2) };
    region.Assign(data, data + ARRAY_SIZE(data));
  }
  TEST_EQUAL(region.Rect(), m2::Rect<typename P::value_type>(0, 0, 8, 9), ());
  TEST(region.Contains(P(0, 0)), ());
  TEST(region.Contains(P(3, 7)), ());
  TEST(region.Contains(P(1, 2)), ());
  TEST(region.Contains(P(1, 1)), ());
  TEST(!region.Contains(P(6, 2)), ());
  TEST(!region.Contains(P(3, 5)), ());
  TEST(!region.Contains(P(5, 8)), ());
  region.ForEachPoint(checker);
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
  Test<m2::RegionD>();
  Test<m2::RegionF>();
  Test<m2::RegionI>();

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
