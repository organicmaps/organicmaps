#include "testing/testing.hpp"

#include "geometry/dbox.hpp"
#include "geometry/point2d.hpp"

using namespace m2;

namespace
{
UNIT_TEST(DBox_Smoke)
{
  {
    DBox dbox;
    TEST(!dbox.IsInside(0, 0), ());
  }

  {
    DBox dbox;
    dbox.Add(0, 0);
    TEST(dbox.IsInside(0, 0), ());
    TEST(!dbox.IsInside(0, 1), ());
    TEST(!dbox.IsInside(1, 0), ());
    TEST(!dbox.IsInside(1, 1), ());
    TEST(!dbox.IsInside(0.5, 0.5), ());

    dbox.Add(1, 1);
    TEST(dbox.IsInside(0, 0), ());
    TEST(dbox.IsInside(1, 1), ());
    TEST(dbox.IsInside(0.5, 0.5), ());
    TEST(!dbox.IsInside(1, 0), ());
    TEST(!dbox.IsInside(0, 1), ());
  }

  {
    DBox dbox;

    dbox.Add(0, 1);
    dbox.Add(0, -1);
    dbox.Add(-1, 0);
    dbox.Add(1, 0);
    TEST(dbox.IsInside(0, 0), ());
    TEST(dbox.IsInside(0.5, 0.5), ());
    TEST(dbox.IsInside(0.5, -0.5), ());
    TEST(dbox.IsInside(-0.5, 0.5), ());
    TEST(dbox.IsInside(-0.5, -0.5), ());

    TEST(!dbox.IsInside(0.51, 0.51), ());
    TEST(!dbox.IsInside(0.51, -0.51), ());
    TEST(!dbox.IsInside(-0.51, 0.51), ());
    TEST(!dbox.IsInside(-0.51, -0.51), ());
  }
}
}  // namespace
