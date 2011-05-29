#include "../../base/SRC_FIRST.hpp"
#include "../../testing/testing.hpp"

#include "../../base/base.hpp"

UNIT_TEST(Assert_Smoke)
{
  int x = 5;
  ASSERT_EQUAL ( x, 5, () );
  ASSERT_NOT_EQUAL ( x, 6, () );
  //ASSERT_EQUAL ( x, 666, ("Skip this to continue test") );
}

UNIT_TEST(Check_Smoke)
{
  int x = 5;
  CHECK_EQUAL ( x, 5, () );
  CHECK_NOT_EQUAL ( x, 6, () );
  //CHECK_EQUAL ( x, 666, ("Skip this to continue test") );
}
