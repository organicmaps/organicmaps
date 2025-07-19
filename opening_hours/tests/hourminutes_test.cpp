#include "testing/testing.hpp"

#include "opening_hours/opening_hours.hpp"

#include <sstream>

namespace om::opening_hours
{
UNIT_TEST(HourMinutes_Basic)
{
  HourMinutes hm;
  TEST(hm.IsEmpty(), ());

  hm.SetHours(2_h);
  hm.SetMinutes(30_min);
  TEST(!hm.IsEmpty(), ());
  TEST_EQUAL(hm.GetHours(), 2_h, ());
  TEST_EQUAL(hm.GetMinutes(), 30_min, ());
}

UNIT_TEST(HourMinutes_IsExtended)
{
  HourMinutes hm;
  hm.SetDuration(25_h);
  TEST(hm.IsExtended(), ());
  hm.SetDuration(23_h + 59_min);
  TEST(!hm.IsExtended(), ());
}

UNIT_TEST(HourMinutes_Output)
{
  HourMinutes hm;
  hm.SetHours(5_h);
  hm.SetMinutes(7_min);

  std::ostringstream oss;
  oss << hm;
  TEST_EQUAL(oss.str(), "05:07", ());
}

UNIT_TEST(HourMinutes_OperatorMinus)
{
  HourMinutes hm;
  hm.SetHours(3_h);
  hm.SetMinutes(15_min);

  HourMinutes neg = -hm;
  TEST_EQUAL(neg.GetHours(), -3_h, ());
  TEST_EQUAL(neg.GetMinutes(), -15_min, ());
}
}  // namespace om::opening_hours