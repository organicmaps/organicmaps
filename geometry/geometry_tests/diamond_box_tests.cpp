#include "testing/testing.hpp"

#include "geometry/diamond_box.hpp"
#include "geometry/point2d.hpp"

using namespace m2;

namespace
{
UNIT_TEST(DiamondBox_Smoke)
{
  {
    DiamondBox dbox;
    TEST(!dbox.HasPoint(0, 0), ());
  }

  {
    DiamondBox dbox;
    dbox.Add(0, 0);
    TEST(dbox.HasPoint(0, 0), ());
    TEST(!dbox.HasPoint(0, 1), ());
    TEST(!dbox.HasPoint(1, 0), ());
    TEST(!dbox.HasPoint(1, 1), ());
    TEST(!dbox.HasPoint(0.5, 0.5), ());

    dbox.Add(1, 1);
    TEST(dbox.HasPoint(0, 0), ());
    TEST(dbox.HasPoint(1, 1), ());
    TEST(dbox.HasPoint(0.5, 0.5), ());
    TEST(!dbox.HasPoint(1, 0), ());
    TEST(!dbox.HasPoint(0, 1), ());
  }

  {
    DiamondBox dbox;

    dbox.Add(0, 1);
    dbox.Add(0, -1);
    dbox.Add(-1, 0);
    dbox.Add(1, 0);
    TEST(dbox.HasPoint(0, 0), ());
    TEST(dbox.HasPoint(0.5, 0.5), ());
    TEST(dbox.HasPoint(0.5, -0.5), ());
    TEST(dbox.HasPoint(-0.5, 0.5), ());
    TEST(dbox.HasPoint(-0.5, -0.5), ());

    TEST(!dbox.HasPoint(0.51, 0.51), ());
    TEST(!dbox.HasPoint(0.51, -0.51), ());
    TEST(!dbox.HasPoint(-0.51, 0.51), ());
    TEST(!dbox.HasPoint(-0.51, -0.51), ());
  }
}
}  // namespace
