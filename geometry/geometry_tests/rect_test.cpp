#include "geometry/rect2d.hpp"
#include "testing/testing.hpp"

UNIT_TEST(Rect_Intersect)
{
  m2::RectD r(0, 0, 100, 100);
  m2::RectD r1(10, 10, 20, 20);

  TEST(r1.IsIntersect(r), ());
  TEST(r.IsIntersect(r1), ());

  m2::RectD r2(-100, -100, -50, -50);

  TEST(!r2.IsIntersect(r), ());
  TEST(!r.IsIntersect(r2), ());

  m2::RectD r3(-10, -10, 10, 10);

  TEST(r3.IsIntersect(r), ());
  TEST(r.IsIntersect(r3), ());
}
