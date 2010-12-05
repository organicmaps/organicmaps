#include "../../testing/testing.hpp"
#include "../string_utils.hpp"

UNIT_TEST(make_lower_case)
{
  string s;

  s = "THIS_IS_UPPER";
  utils::make_lower_case(s);
  TEST_EQUAL(s, "this_is_upper", ());

  s = "THIS_iS_MiXed";
  utils::make_lower_case(s);
  TEST_EQUAL(s, "this_is_mixed", ());

  s = "this_is_lower";
  utils::make_lower_case(s);
  TEST_EQUAL(s, "this_is_lower", ());
}
