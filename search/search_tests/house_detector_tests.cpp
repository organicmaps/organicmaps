#include "../../testing/testing.hpp"
#include "../house_detector.hpp"

UNIT_TEST(LESS_WITH_EPSILON)
{
  search::HouseDetector::LessWithEpsilon compare;
  {
    m2::PointD a(1, 1);
    m2::PointD b(2, 2);
    TEST(compare(a, b), ());
    TEST(!compare(b, a), ());
  }
  {
    m2::PointD a(1, 1);
    m2::PointD b(1, 1);
    TEST(!compare(a, b), ());
    TEST(!compare(b, a), ());
  }
  {
    m2::PointD a(1, 1);
    m2::PointD b(1.1, 1.1);
    TEST(compare(a, b), ());
    TEST(!compare(b, a), ());
  }
  {
    m2::PointD a(1, 1);
    m2::PointD b(1 + search::s_epsilon, 1);
    TEST(!compare(a, b), ());
    TEST(!compare(b, a), ());
  }
  {
    m2::PointD a(1, 1);
    m2::PointD b(1 + search::s_epsilon, 1 - search::s_epsilon);
    TEST(!compare(a, b), ());
    TEST(!compare(b, a), ());
  }
  {
    m2::PointD a(1, 1 - search::s_epsilon);
    m2::PointD b(1 + search::s_epsilon, 1);
    TEST(!compare(a, b), ());
    TEST(!compare(b, a), ());
  }
  {
    m2::PointD a(1 - search::s_epsilon, 1 - search::s_epsilon);
    m2::PointD b(1, 1);
    TEST(!compare(a, b), ());
    TEST(!compare(b, a), ());
  }
  {
    m2::PointD a(1, 1);
    m2::PointD b(1 + search::s_epsilon, 1 + search::s_epsilon);
    TEST(!compare(a, b), ());
    TEST(!compare(b, a), ());
  }
  {
    m2::PointD a(1, 1);
    m2::PointD b(1 + 2 * search::s_epsilon, 1 + search::s_epsilon);
    TEST(compare(a, b), ());
    TEST(!compare(b, a), ());
  }
}
