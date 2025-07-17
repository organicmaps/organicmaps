#include "testing/testing.hpp"

#include "coding/move_to_front.hpp"

#include <cstdint>

using namespace coding;

namespace
{
UNIT_TEST(MoveToFront_Smoke)
{
  MoveToFront mtf;
  for (size_t i = 0; i < 256; ++i)
    TEST_EQUAL(mtf[i], i, ());

  // Initially 3 should be on the 3rd position.
  TEST_EQUAL(mtf.Transform(3), 3, ());

  // After the first transform, 3 should be moved to the 0th position.
  TEST_EQUAL(mtf.Transform(3), 0, ());
  TEST_EQUAL(mtf.Transform(3), 0, ());
  TEST_EQUAL(mtf.Transform(3), 0, ());

  TEST_EQUAL(mtf[0], 3, ());
  TEST_EQUAL(mtf[1], 0, ());
  TEST_EQUAL(mtf[2], 1, ());
  TEST_EQUAL(mtf[3], 2, ());
  for (size_t i = 4; i < 256; ++i)
    TEST_EQUAL(mtf[i], i, ());
}
}  // namespace
