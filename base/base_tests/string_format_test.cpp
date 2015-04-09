#include "testing/testing.hpp"
#include "base/string_format.hpp"

UNIT_TEST(StringFormat_Smoke)
{
  TEST_EQUAL(strings::Format("this is ^ ^ ^ ^", "a", "very", "simple", "test"), "this is a very simple test", ());

  TEST_EQUAL(strings::Format("this", "a"), "this", ());

  TEST_EQUAL(strings::Format("this ^ ^", "is"), "this is ^", ());
}
