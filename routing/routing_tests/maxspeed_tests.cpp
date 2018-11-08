#include "testing/testing.hpp"

#include "routing/maxspeed_conversion.hpp"
#include "routing/maxspeed_serialization.hpp"
#include "routing/maxspeeds.hpp"

#include "coding/file_name_utils.hpp"
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

void TestMaxspeedSerialization(std::vector<FeatureMaxspeed> const & speeds)
{
  vector<char> buffer;
  MemWriter<vector<char>> w(buffer);

  MaxspeedSerializer::Serialize(speeds, w);

  size_t const sz = buffer.size();

  MemReader r(buffer.data(), sz);
  ReaderSource<MemReader> src(r);
  Maxspeeds maxspeeds;
  MaxspeedSerializer::Deserialize(src, maxspeeds);

  for (auto const s : speeds)
  {
    TEST(maxspeeds.HasMaxspeed(s.GetFeatureId()), (s));
    TEST_EQUAL(maxspeeds.GetMaxspeed(s.GetFeatureId()), s.GetMaxspeed(), (s));
  }
}

UNIT_TEST(MaxspeedConverter)
{
  auto const & conv = GetMaxspeedConverter();

  // All maxspeed limits conversion test.
  for (size_t i = 0; i < numeric_limits<uint8_t>::max(); ++i)
  {
    auto const macro = static_cast<SpeedMacro>(i);
    auto const speed = conv.MacroToSpeed(macro);
    auto const backToMacro = conv.SpeedToMacro(speed);
    if (speed.IsValid())
      TEST_EQUAL(backToMacro, macro, (i));
    else
      TEST_EQUAL(backToMacro, SpeedMacro::Undefined, (i));
  }

  // Test on conversion some maxspeed value to macro.
  TEST_EQUAL(conv.SpeedToMacro(SpeedInUnits(kInvalidSpeed, Units::Metric)), SpeedMacro::Undefined, ());
  TEST_EQUAL(conv.SpeedToMacro(SpeedInUnits(kNoneMaxSpeed, Units::Metric)), SpeedMacro::None, ());
  TEST_EQUAL(conv.SpeedToMacro(SpeedInUnits(kWalkMaxSpeed, Units::Metric)), SpeedMacro::Walk, ());
  TEST_EQUAL(conv.SpeedToMacro(SpeedInUnits(60, Units::Metric)), SpeedMacro::Speed60kph, ());
  TEST_EQUAL(conv.SpeedToMacro(SpeedInUnits(90, Units::Metric)), SpeedMacro::Speed90kph, ());
  TEST_EQUAL(conv.SpeedToMacro(SpeedInUnits(30, Units::Imperial)), SpeedMacro::Speed30mph, ());
  TEST_EQUAL(conv.SpeedToMacro(SpeedInUnits(33, Units::Metric)), SpeedMacro::Undefined, ());

  // Test on conversion some maxspeed to macro to value.
  TEST_EQUAL(conv.MacroToSpeed(SpeedMacro::Undefined), SpeedInUnits(kInvalidSpeed, Units::Metric), ());
  TEST_EQUAL(conv.MacroToSpeed(SpeedMacro::None), SpeedInUnits(kNoneMaxSpeed, Units::Metric), ());
  TEST_EQUAL(conv.MacroToSpeed(SpeedMacro::Walk), SpeedInUnits(kWalkMaxSpeed, Units::Metric), ());
  TEST_EQUAL(conv.MacroToSpeed(SpeedMacro::Speed60kph), SpeedInUnits(60, Units::Metric), ());
  TEST_EQUAL(conv.MacroToSpeed(SpeedMacro::Speed90kph), SpeedInUnits(90, Units::Metric), ());
  TEST_EQUAL(conv.MacroToSpeed(SpeedMacro::Speed30mph), SpeedInUnits(30, Units::Imperial), ());

  // Test on IsValidMacro() method.
  TEST(!conv.IsValidMacro(0), ());
  TEST(conv.IsValidMacro(1), ()); // static_cast<uint8_t>(None) == 1
  TEST(!conv.IsValidMacro(9), ()); // A value which is undefined in SpeedMacro enum class.
}

UNIT_TEST(MaxspeedSerializer_Smoke)
{
  TestMaxspeedSerialization({});
}

UNIT_TEST(MaxspeedSerializer_OneForwardMetric)
{
  TestMaxspeedSerialization(
      {FeatureMaxspeed(0 /* feature id */, Units::Metric, 20 /* speed */)});
}

UNIT_TEST(MaxspeedSerializer_OneNone)
{
  TestMaxspeedSerialization(
      {FeatureMaxspeed(0 /* feature id */, Units::Metric, kNoneMaxSpeed)});
}

UNIT_TEST(MaxspeedSerializer_OneWalk)
{
  TestMaxspeedSerialization(
      {FeatureMaxspeed(0 /* feature id */, Units::Metric, kWalkMaxSpeed)});
}

UNIT_TEST(MaxspeedSerializer_OneBidirectionalMetric_1)
{
  TestMaxspeedSerialization(
      {FeatureMaxspeed(0 /* feature id */, Units::Metric, 20 /* speed */, 40 /* speed */)});
}

UNIT_TEST(MaxspeedSerializer_OneBidirectionalMetric_2)
{
  TestMaxspeedSerialization(
      {FeatureMaxspeed(0 /* feature id */, Units::Metric, 10 /* speed */, kWalkMaxSpeed)});
}

UNIT_TEST(MaxspeedSerializer_OneBidirectionalImperial)
{
  TestMaxspeedSerialization(
      {FeatureMaxspeed(0 /* feature id */, Units::Imperial, 30 /* speed */, 50 /* speed */)});
}

UNIT_TEST(MaxspeedSerializer_BigMetric)
{
  std::vector<FeatureMaxspeed> const maxspeeds = {
      {0 /* feature id */, Units::Metric, 20 /* speed */},
      {1 /* feature id */, Units::Metric, 60 /* speed */},
      {4 /* feature id */, Units::Metric, 90 /* speed */},
      {5 /* feature id */, Units::Metric, 5 /* speed */},
      {7 /* feature id */, Units::Metric, 70 /* speed */, 90 /* speed */},
      {8 /* feature id */, Units::Metric, 100 /* speed */},
      {9 /* feature id */, Units::Metric, 60 /* speed */},
      {10 /* feature id */, Units::Metric, kNoneMaxSpeed},
      {11 /* feature id */, Units::Metric, 40 /* speed */, 50 /* speed */},
      {12 /* feature id */, Units::Metric, 40 /* speed */, 60 /* speed */},
  };
  TestMaxspeedSerialization(maxspeeds);
}

UNIT_TEST(MaxspeedSerializer_BigImperial)
{
  std::vector<FeatureMaxspeed> const maxspeeds = {
      {0 /* feature id */, Units::Imperial, 30 /* speed */},
      {1 /* feature id */, Units::Imperial, 5 /* speed */},
      {4 /* feature id */, Units::Imperial, 3 /* speed */},
      {5 /* feature id */, Units::Imperial, 5 /* speed */},
      {7 /* feature id */, Units::Imperial, 30 /* speed */, 50 /* speed */},
      {8 /* feature id */, Units::Imperial, 70 /* speed */},
      {9 /* feature id */, Units::Imperial, 50 /* speed */},
      {10 /* feature id */, Units::Imperial, 40 /* speed */, 50 /* speed */},
      {11 /* feature id */, Units::Imperial, 30 /* speed */, 50 /* speed */},
  };
  TestMaxspeedSerialization(maxspeeds);
}
}  // namespace
