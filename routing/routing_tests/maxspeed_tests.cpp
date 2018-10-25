#include "testing/testing.hpp"

#include "routing/maxspeed_conversion.hpp"
#include "routing/maxspeed_serialization.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "platform/measurement_utils.hpp"

#include <cstdint>
#include <limits>
#include <vector>

namespace
{
using namespace measurement_utils;
using namespace routing;
using namespace std;

void TestMaxspeedSerialization(std::vector<FeatureMaxspeed> const & speed)
{
  vector<char> buffer;
  MemWriter<vector<char> > w(buffer);

  MaxspeedSerializer::Serialize(speed, w);

  size_t const sz = buffer.size();

  MemReader r(buffer.data(), sz);
  ReaderSource<MemReader> src(r);
  std::vector<FeatureMaxspeed> result;
  MaxspeedSerializer::Deserialize(src, result);

  TEST_EQUAL(speed, result, ());
}

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

UNIT_TEST(MaxspeedSerializer_Smoke)
{
  TestMaxspeedSerialization({});
}

UNIT_TEST(MaxspeedSerializer_OneForwardMetric)
{
  TestMaxspeedSerialization(
      {FeatureMaxspeed(0 /* feature id */, SpeedInUnits(20 /* speed */, Units::Metric))});
}

UNIT_TEST(MaxspeedSerializer_OneNone)
{
  TestMaxspeedSerialization(
      {FeatureMaxspeed(0 /* feature id */, SpeedInUnits(kNoneMaxSpeed, Units::Metric))});
}

UNIT_TEST(MaxspeedSerializer_OneWalk)
{
  TestMaxspeedSerialization(
      {FeatureMaxspeed(0 /* feature id */, SpeedInUnits(kWalkMaxSpeed, Units::Metric))});
}

UNIT_TEST(MaxspeedSerializer_OneBidirectionalMetric_1)
{
  TestMaxspeedSerialization(
      {FeatureMaxspeed(0 /* feature id */, SpeedInUnits(20 /* speed */, Units::Metric),
                       SpeedInUnits(40 /* speed */, Units::Metric))});
}

UNIT_TEST(MaxspeedSerializer_OneBidirectionalMetric_2)
{
  TestMaxspeedSerialization(
      {FeatureMaxspeed(0 /* feature id */, SpeedInUnits(10 /* speed */, Units::Metric),
                       SpeedInUnits(kWalkMaxSpeed, Units::Metric))});
}

UNIT_TEST(MaxspeedSerializer_OneBidirectionalImperial)
{
  TestMaxspeedSerialization(
      {FeatureMaxspeed(0 /* feature id */, SpeedInUnits(30 /* speed */, Units::Imperial),
                       SpeedInUnits(50 /* speed */, Units::Imperial))});
}

UNIT_TEST(MaxspeedSerializer_BigMetric)
{
  std::vector<FeatureMaxspeed> const maxspeeds = {
      FeatureMaxspeed(0 /* feature id */, SpeedInUnits(20 /* speed */, Units::Metric)),
      FeatureMaxspeed(1 /* feature id */, SpeedInUnits(60 /* speed */, Units::Metric)),
      FeatureMaxspeed(4 /* feature id */, SpeedInUnits(90 /* speed */, Units::Metric)),
      FeatureMaxspeed(5 /* feature id */, SpeedInUnits(5 /* speed */, Units::Metric)),
      FeatureMaxspeed(7 /* feature id */, SpeedInUnits(70 /* speed */, Units::Metric),
                      SpeedInUnits(90 /* speed */, Units::Metric)),
      FeatureMaxspeed(8 /* feature id */, SpeedInUnits(100 /* speed */, Units::Metric)),
      FeatureMaxspeed(9 /* feature id */, SpeedInUnits(60 /* speed */, Units::Metric)),
      FeatureMaxspeed(10 /* feature id */, SpeedInUnits(kNoneMaxSpeed, Units::Metric)),
      FeatureMaxspeed(11 /* feature id */, SpeedInUnits(40 /* speed */, Units::Metric),
                      SpeedInUnits(50 /* speed */, Units::Metric)),
      FeatureMaxspeed(12 /* feature id */, SpeedInUnits(40 /* speed */, Units::Metric),
                      SpeedInUnits(60 /* speed */, Units::Metric)),
  };
  TestMaxspeedSerialization(maxspeeds);
}

UNIT_TEST(MaxspeedSerializer_BigImperial)
{
  std::vector<FeatureMaxspeed> const maxspeeds = {
      FeatureMaxspeed(0 /* feature id */, SpeedInUnits(30 /* speed */, Units::Imperial)),
      FeatureMaxspeed(1 /* feature id */, SpeedInUnits(5 /* speed */, Units::Imperial)),
      FeatureMaxspeed(4 /* feature id */, SpeedInUnits(1 /* speed */, Units::Imperial)),
      FeatureMaxspeed(5 /* feature id */, SpeedInUnits(5 /* speed */, Units::Imperial)),
      FeatureMaxspeed(7 /* feature id */, SpeedInUnits(30 /* speed */, Units::Imperial),
                      SpeedInUnits(50 /* speed */, Units::Imperial)),
      FeatureMaxspeed(8 /* feature id */, SpeedInUnits(70 /* speed */, Units::Imperial)),
      FeatureMaxspeed(9 /* feature id */, SpeedInUnits(50 /* speed */, Units::Imperial)),
      FeatureMaxspeed(10 /* feature id */, SpeedInUnits(40 /* speed */, Units::Imperial),
                      SpeedInUnits(50 /* speed */, Units::Metric)),
      FeatureMaxspeed(11 /* feature id */, SpeedInUnits(30 /* speed */, Units::Imperial),
                      SpeedInUnits(50 /* speed */, Units::Metric)),
  };
  TestMaxspeedSerialization(maxspeeds);
}
}  // namespace
