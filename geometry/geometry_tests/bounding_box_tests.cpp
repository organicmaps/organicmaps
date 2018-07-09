#include "testing/testing.hpp"

#include "geometry/bounding_box.hpp"
#include "geometry/point2d.hpp"

using namespace m2;

namespace
{
UNIT_TEST(BoundingBox_Smoke)
{
  {
    BoundingBox bbox;

    TEST(!bbox.HasPoint(0, 0), ());
    TEST(!bbox.HasPoint(-1, 1), ());
  }

  {
    BoundingBox bbox;

    bbox.Add(0, 0);
    TEST(bbox.HasPoint(0, 0), ());
    TEST(!bbox.HasPoint(1, 0), ());
    TEST(!bbox.HasPoint(0, 1), ());
    TEST(!bbox.HasPoint(1, 1), ());
    TEST(!bbox.HasPoint(0.5, 0.5), ());

    bbox.Add(1, 1);
    TEST(bbox.HasPoint(1, 0), ());
    TEST(bbox.HasPoint(0, 1), ());
    TEST(bbox.HasPoint(1, 1), ());
    TEST(bbox.HasPoint(0.5, 0.5), ());
  }
}
}  // namespace
