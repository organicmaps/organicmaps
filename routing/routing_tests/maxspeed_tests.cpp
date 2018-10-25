#include "testing/testing.hpp"

#include "routing/maxspeed_conversion.hpp"

#include "platform/measurement_utils.hpp"

#include <cstdint>
#include <limits>

namespace
{
using namespace measurement_utils;
using namespace routing;
using namespace std;

UNIT_TEST(MaxspeedConverter)
{
  auto const & conv = GetMaxspeedConverter();

  // All maxspeed limits conversion test.
  for (size_t i = 0; i < numeric_limits<uint8_t>::max(); ++i)
  {
    auto const macro = static_cast<Maxspeed>(i);
    auto const speed = conv.MacroToSpeed(macro);
    auto const backToMacro = conv.SpeedToMacro(speed);
    if (speed.IsValid())
      TEST_EQUAL(backToMacro, macro, (i));
    else
      TEST_EQUAL(backToMacro, Maxspeed::Undefined, (i));
  }

  // Test on conversion some maxspeed value to macro.
  TEST_EQUAL(conv.SpeedToMacro(SpeedInUnits(kInvalidSpeed, Units::Metric)), Maxspeed::Undefined, ());
  TEST_EQUAL(conv.SpeedToMacro(SpeedInUnits(kNoneMaxSpeed, Units::Metric)), Maxspeed::None, ());
  TEST_EQUAL(conv.SpeedToMacro(SpeedInUnits(kWalkMaxSpeed, Units::Metric)), Maxspeed::Walk, ());
  TEST_EQUAL(conv.SpeedToMacro(SpeedInUnits(60, Units::Metric)), Maxspeed::Speed60kph, ());
  TEST_EQUAL(conv.SpeedToMacro(SpeedInUnits(90, Units::Metric)), Maxspeed::Speed90kph, ());
  TEST_EQUAL(conv.SpeedToMacro(SpeedInUnits(30, Units::Imperial)), Maxspeed::Speed30mph, ());
  TEST_EQUAL(conv.SpeedToMacro(SpeedInUnits(33, Units::Metric)), Maxspeed::Undefined, ());

  // Test on conversion some maxspeed to macro to value.
  TEST_EQUAL(conv.MacroToSpeed(Maxspeed::Undefined), SpeedInUnits(kInvalidSpeed, Units::Metric), ());
  TEST_EQUAL(conv.MacroToSpeed(Maxspeed::None), SpeedInUnits(kNoneMaxSpeed, Units::Metric), ());
  TEST_EQUAL(conv.MacroToSpeed(Maxspeed::Walk), SpeedInUnits(kWalkMaxSpeed, Units::Metric), ());
  TEST_EQUAL(conv.MacroToSpeed(Maxspeed::Speed60kph), SpeedInUnits(60, Units::Metric), ());
  TEST_EQUAL(conv.MacroToSpeed(Maxspeed::Speed90kph), SpeedInUnits(90, Units::Metric), ());
  TEST_EQUAL(conv.MacroToSpeed(Maxspeed::Speed30mph), SpeedInUnits(30, Units::Imperial), ());

  // Test on IsValidMacro() method.
  TEST(!conv.IsValidMacro(0), ());
  TEST(conv.IsValidMacro(1), ()); // static_cast<uint8_t>(None) == 1
  TEST(!conv.IsValidMacro(9), ()); // A value which is undefined in Maxspeed enum class.
}
}  // namespace
