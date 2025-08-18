#include "testing/testing.hpp"

#include "routing/maxspeeds.hpp"
#include "routing/maxspeeds_serialization.hpp"

#include "routing_common/maxspeed_conversion.hpp"

#include "platform/measurement_utils.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "base/file_name_utils.hpp"

#include <cstdint>
#include <limits>
#include <vector>

namespace maxspeeds_test
{
using namespace measurement_utils;
using namespace routing;
using namespace std;

void TestMaxspeedsSerialization(vector<FeatureMaxspeed> const & speeds)
{
  vector<char> buffer;
  MemWriter<vector<char>> w(buffer);

  std::vector<MaxspeedsSerializer::FeatureSpeedMacro> inputSpeeds;
  inputSpeeds.reserve(speeds.size());
  MaxspeedConverter const & converter = GetMaxspeedConverter();
  for (auto const & s : speeds)
  {
    inputSpeeds.push_back({s.GetFeatureId(), converter.SpeedToMacro(s.GetForwardSpeedInUnits()),
                           converter.SpeedToMacro(s.GetBackwardSpeedInUnits())});
  }

  int constexpr SPEEDS_COUNT = MaxspeedsSerializer::DEFAULT_SPEEDS_COUNT;
  SpeedInUnits defSpeeds[SPEEDS_COUNT];
  MaxspeedsSerializer::HW2SpeedMap defaultMap[SPEEDS_COUNT];

  if (!speeds.empty())
  {
    defSpeeds[0] = speeds.front().GetForwardSpeedInUnits();
    defSpeeds[1] = speeds.back().GetForwardSpeedInUnits();
    defaultMap[0][HighwayType::HighwayPrimary] = converter.SpeedToMacro(defSpeeds[0]);
    defaultMap[1][HighwayType::HighwaySecondary] = converter.SpeedToMacro(defSpeeds[1]);
  }

  MaxspeedsSerializer::Serialize(inputSpeeds, defaultMap, w);

  size_t const sz = buffer.size();

  MemReader r(buffer.data(), sz);
  ReaderSource<MemReader> src(r);
  Maxspeeds maxspeeds;
  MaxspeedsSerializer::Deserialize(src, maxspeeds);

  for (auto const & s : speeds)
    TEST_EQUAL(maxspeeds.GetMaxspeed(s.GetFeatureId()), s.GetMaxspeed(), (s));

  TEST_EQUAL(maxspeeds.GetDefaultSpeed(false, HighwayType::HighwayPrimary), defSpeeds[0].GetSpeed(), ());
  TEST_EQUAL(maxspeeds.GetDefaultSpeed(true, HighwayType::HighwaySecondary), defSpeeds[1].GetSpeed(), ());
}

UNIT_TEST(MaxspeedConverter_Smoke)
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
  TEST_EQUAL(conv.SpeedToMacro(SpeedInUnits(60, Units::Metric)), SpeedMacro::Speed60KmPH, ());
  TEST_EQUAL(conv.SpeedToMacro(SpeedInUnits(90, Units::Metric)), SpeedMacro::Speed90KmPH, ());
  TEST_EQUAL(conv.SpeedToMacro(SpeedInUnits(30, Units::Imperial)), SpeedMacro::Speed30MPH, ());
  TEST_EQUAL(conv.SpeedToMacro(SpeedInUnits(33, Units::Metric)), SpeedMacro::Undefined, ());

  // Test on conversion some maxspeed to macro to value.
  TEST_EQUAL(conv.MacroToSpeed(SpeedMacro::Undefined), SpeedInUnits(kInvalidSpeed, Units::Metric), ());
  TEST_EQUAL(conv.MacroToSpeed(SpeedMacro::None), SpeedInUnits(kNoneMaxSpeed, Units::Metric), ());
  TEST_EQUAL(conv.MacroToSpeed(SpeedMacro::Walk), SpeedInUnits(kWalkMaxSpeed, Units::Metric), ());
  TEST_EQUAL(conv.MacroToSpeed(SpeedMacro::Speed60KmPH), SpeedInUnits(60, Units::Metric), ());
  TEST_EQUAL(conv.MacroToSpeed(SpeedMacro::Speed90KmPH), SpeedInUnits(90, Units::Metric), ());
  TEST_EQUAL(conv.MacroToSpeed(SpeedMacro::Speed30MPH), SpeedInUnits(30, Units::Imperial), ());
}

UNIT_TEST(MaxspeedConverter_ClosestValidMacro)
{
  auto const & conv = GetMaxspeedConverter();

  SpeedInUnits expected{1, Units::Metric};
  TEST_EQUAL(conv.ClosestValidMacro({0, Units::Metric}), expected, ());
  TEST_EQUAL(conv.ClosestValidMacro(expected), expected, ());

  expected = {380, Units::Metric};
  TEST_EQUAL(conv.ClosestValidMacro(expected), expected, ());
  TEST_EQUAL(conv.ClosestValidMacro({400, Units::Metric}), expected, ());

  expected = {3, Units::Imperial};
  TEST_EQUAL(conv.ClosestValidMacro({0, Units::Imperial}), expected, ());
  TEST_EQUAL(conv.ClosestValidMacro({1, Units::Imperial}), expected, ());
  TEST_EQUAL(conv.ClosestValidMacro(expected), expected, ());

  expected = {125, Units::Imperial};
  TEST_EQUAL(conv.ClosestValidMacro(expected), expected, ());
  TEST_EQUAL(conv.ClosestValidMacro({150, Units::Imperial}), expected, ());

  expected = {50, Units::Metric};
  TEST_EQUAL(conv.ClosestValidMacro({48, Units::Metric}), expected, ());
  TEST_EQUAL(conv.ClosestValidMacro({52, Units::Metric}), expected, ());

  expected = {40, Units::Imperial};
  TEST_EQUAL(conv.ClosestValidMacro({42, Units::Imperial}), expected, ());
  TEST_EQUAL(conv.ClosestValidMacro({38, Units::Imperial}), expected, ());
}

UNIT_TEST(MaxspeedsSerializer_Smoke)
{
  TestMaxspeedsSerialization({});
}

UNIT_TEST(MaxspeedsSerializer_OneForwardMetric)
{
  TestMaxspeedsSerialization({FeatureMaxspeed(0 /* feature id */, Units::Metric, 20 /* speed */)});
}

UNIT_TEST(MaxspeedsSerializer_OneNone)
{
  TestMaxspeedsSerialization({FeatureMaxspeed(0 /* feature id */, Units::Metric, kNoneMaxSpeed)});
}

UNIT_TEST(MaxspeedsSerializer_OneWalk)
{
  TestMaxspeedsSerialization({FeatureMaxspeed(0 /* feature id */, Units::Metric, kWalkMaxSpeed)});
}

UNIT_TEST(MaxspeedsSerializer_OneBidirectionalMetric_1)
{
  TestMaxspeedsSerialization({FeatureMaxspeed(0 /* feature id */, Units::Metric, 20 /* speed */, 40 /* speed */)});
}

UNIT_TEST(MaxspeedsSerializer_OneBidirectionalMetric_2)
{
  TestMaxspeedsSerialization({FeatureMaxspeed(0 /* feature id */, Units::Metric, 10 /* speed */, kWalkMaxSpeed)});
}

UNIT_TEST(MaxspeedsSerializer_OneBidirectionalImperial)
{
  TestMaxspeedsSerialization({FeatureMaxspeed(0 /* feature id */, Units::Imperial, 30 /* speed */, 50 /* speed */)});
}

UNIT_TEST(MaxspeedsSerializer_BigMetric)
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
  TestMaxspeedsSerialization(maxspeeds);
}

UNIT_TEST(MaxspeedsSerializer_BigImperial)
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
  TestMaxspeedsSerialization(maxspeeds);
}

UNIT_TEST(Maxspeed_Smoke)
{
  {
    Maxspeed maxspeed;
    TEST(!maxspeed.IsValid(), ());
    TEST(!maxspeed.IsBidirectional(), ());
    TEST_EQUAL(maxspeed.GetSpeedInUnits(true /* forward */), kInvalidSpeed, ());
    TEST_EQUAL(maxspeed.GetSpeedKmPH(true /* forward */), kInvalidSpeed, ());
    TEST_EQUAL(maxspeed.GetSpeedInUnits(false /* forward */), kInvalidSpeed, ());
    TEST_EQUAL(maxspeed.GetSpeedKmPH(false /* forward */), kInvalidSpeed, ());
  }

  {
    Maxspeed maxspeed = {Units::Metric, 20 /* forward */, kInvalidSpeed /* backward */};
    TEST(maxspeed.IsValid(), ());
    TEST(!maxspeed.IsBidirectional(), ());
    TEST_EQUAL(maxspeed.GetSpeedInUnits(true /* forward */), 20, ());
    TEST_EQUAL(maxspeed.GetSpeedKmPH(true /* forward */), 20, ());
    TEST_EQUAL(maxspeed.GetSpeedInUnits(false /* forward */), 20, ());
    TEST_EQUAL(maxspeed.GetSpeedKmPH(false /* forward */), 20, ());
  }

  {
    Maxspeed maxspeed = {Units::Metric, 30 /* forward */, 40 /* backward */};
    TEST(maxspeed.IsValid(), ());
    TEST(maxspeed.IsBidirectional(), ());
    TEST_EQUAL(maxspeed.GetSpeedInUnits(true /* forward */), 30, ());
    TEST_EQUAL(maxspeed.GetSpeedKmPH(true /* forward */), 30, ());
    TEST_EQUAL(maxspeed.GetSpeedInUnits(false /* forward */), 40, ());
    TEST_EQUAL(maxspeed.GetSpeedKmPH(false /* forward */), 40, ());
  }
}
}  // namespace maxspeeds_test
