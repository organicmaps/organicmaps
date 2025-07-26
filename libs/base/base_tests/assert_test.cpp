#include "testing/testing.hpp"

#include "base/base.hpp"
#include "base/exception.hpp"
#include "base/logging.hpp"

UNIT_TEST(Assert_Smoke)
{
  int x = 5;
  // to avoid warning in release
#ifdef RELEASE
  UNUSED_VALUE(x);
#endif
  ASSERT_EQUAL(x, 5, ());
  ASSERT_NOT_EQUAL(x, 6, ());
  // ASSERT_EQUAL ( x, 666, ("Skip this to continue test") );
}

UNIT_TEST(Check_Smoke)
{
  int x = 5;
  CHECK_EQUAL(x, 5, ());
  CHECK_NOT_EQUAL(x, 6, ());
  // CHECK_EQUAL ( x, 666, ("Skip this to continue test") );
}

UNIT_TEST(Exception_Formatting)
{
  try
  {
    MYTHROW(RootException, ("String1", "String2", "String3"));
  }
  catch (RootException const & e)
  {
    LOG(LINFO, ("Exception string: ", e.what()));
  }
}
