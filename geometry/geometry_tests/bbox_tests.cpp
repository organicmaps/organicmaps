#include "testing/testing.hpp"

#include "geometry/bbox.hpp"
#include "geometry/point2d.hpp"

using namespace m2;

namespace
{
UNIT_TEST(BBox_Smoke)
{
  {
    BBox bbox;
    TEST(!bbox.IsInside(0, 0), ());
    TEST(!bbox.IsInside(-1, 1), ());
  }

  {
    BBox bbox;

    bbox.Add(0, 0);
    TEST(bbox.IsInside(0, 0), ());
    TEST(!bbox.IsInside(1, 0), ());
    TEST(!bbox.IsInside(0, 1), ());
    TEST(!bbox.IsInside(1, 1), ());
    TEST(!bbox.IsInside(0.5, 0.5), ());

    bbox.Add(1, 1);
    TEST(bbox.IsInside(1, 0), ());
    TEST(bbox.IsInside(0, 1), ());
    TEST(bbox.IsInside(1, 1), ());
    TEST(bbox.IsInside(0.5, 0.5), ());
  }
}
}  // namespace
