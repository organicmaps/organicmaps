#include "../../testing/testing.hpp"
#include "../macros.hpp"
#include "../stl_add.hpp"

UNIT_TEST(IsSorted)
{
  TEST(IsSorted(static_cast<int*>(0), static_cast<int*>(0)), ());
  int v1[] = { 1, 3, 5 };
  int const v2[] = { 1, 3, 2 };
  TEST(!IsSorted(&v2[0], &v2[0] + ARRAY_SIZE(v2)), ());
  TEST(IsSorted(&v1[0], &v1[0] + ARRAY_SIZE(v1)), ());
  TEST(IsSorted(&v1[0], &v1[0] + 0), ());
  TEST(IsSorted(&v1[0], &v1[0] + 1), ());
  TEST(IsSorted(&v1[0], &v1[0] + 2), ());
}
