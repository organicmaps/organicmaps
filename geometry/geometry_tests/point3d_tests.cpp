#include "testing/testing.hpp"

#include "geometry/point3d.hpp"

using namespace std;

UNIT_TEST(Point3d_DotProduct)
{
  m3::Point<int> p1(1, 4, 3);
  m3::Point<int> p2(1, 2, 3);
  m3::Point<int> p3(1, 0, 3);

  TEST_EQUAL(m3::DotProduct(p1, p2), 18, ());
  TEST_EQUAL(m3::DotProduct(p2, p3), 10, ());
  TEST_EQUAL(m3::DotProduct(p3, p2), 10, ());
  TEST_EQUAL(m3::DotProduct(p1, p3), 10, ());
}

UNIT_TEST(Point3d_CrossProduct_1)
{
  m3::Point<int> p1(1, 0, 0);
  m3::Point<int> p2(0, 1, 0);
  m3::Point<int> p3(0, 0, 1);

  TEST_EQUAL(m3::CrossProduct(p1, p2), p3, ());
  TEST_EQUAL(m3::CrossProduct(p2, p3), p1, ());
  TEST_EQUAL(m3::CrossProduct(p3, p1), p2, ());
}

UNIT_TEST(Point3d_CrossProduct_2)
{
  m3::Point<int> p1(1, 2, 3);
  m3::Point<int> p2(4, 5, 6);

  TEST_EQUAL(m3::CrossProduct(p1, p2), m3::Point<int>(-3, 6, -3), ());
}

UNIT_TEST(Point3d_CrossProduct_3)
{
  m3::Point<int> p1(3, 7, 1);
  m3::Point<int> p2(6, 2, 9);

  TEST_EQUAL(m3::CrossProduct(p1, p2), m3::Point<int>(61, -21, -36), ());
}
