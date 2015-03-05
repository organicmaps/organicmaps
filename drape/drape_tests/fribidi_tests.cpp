#include "base/SRC_FIRST.hpp"
#include "testing/testing.hpp"

#include "drape/fribidi.hpp"


UNIT_TEST(FribidiDirection)
{
  string base = "\u0686\u0631\u0645\u0647\u064A\u0646";
  strings::UniString in = strings::MakeUniString(base);
  strings::UniString out1 = fribidi::log2vis(in);
  string out = "\uFEE6\uFEF4\uFEEC\uFEE3\uFEAE\uFB7C";
  strings::UniString out2 = strings::MakeUniString(out);
  bool eq = out1 == out2;
  TEST_EQUAL(eq, true, ());
}
