#include "../../testing/testing.hpp"
#include "../string_utils.hpp"

UNIT_TEST(make_lower_case)
{
  string s;

  s = "THIS_IS_UPPER";
  strings::make_lower_case(s);
  TEST_EQUAL(s, "this_is_upper", ());

  s = "THIS_iS_MiXed";
  strings::make_lower_case(s);
  TEST_EQUAL(s, "this_is_mixed", ());

  s = "this_is_lower";
  strings::make_lower_case(s);
  TEST_EQUAL(s, "this_is_lower", ());
}

UNIT_TEST(to_double)
{
  string s;
  double d;

  s = "0.123";
  TEST(strings::to_double(s, d), ());
  TEST_ALMOST_EQUAL(0.123, d, ());

  s = "1.";
  TEST(strings::to_double(s, d), ());
  TEST_ALMOST_EQUAL(1.0, d, ());

  s = "0";
  TEST(strings::to_double(s, d), ());
  TEST_ALMOST_EQUAL(0., d, ());

  s = "5.6843418860808e-14";
  TEST(strings::to_double(s, d), ());
  TEST_ALMOST_EQUAL(5.6843418860808e-14, d, ());

  s = "-2";
  TEST(strings::to_double(s, d), ());
  TEST_ALMOST_EQUAL(-2.0, d, ());
}

UNIT_TEST(to_string)
{
  TEST_EQUAL(strings::to_string(-1), "-1", ());
  TEST_EQUAL(strings::to_string(1234567890), "1234567890", ());
  TEST_EQUAL(strings::to_string(0.56), "0.56", ());
  TEST_EQUAL(strings::to_string(-100.2), "-100.2", ());
}
