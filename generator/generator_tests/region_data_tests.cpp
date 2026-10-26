#include "testing/testing.hpp"

#include "generator/region_meta.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "timezone/serdes.hpp"

#include <functional>
#include <optional>
#include <vector>

using namespace feature;
using SpeedLimitSignType = RegionData::SpeedLimitSignType;

void RunTestSamples(std::function<void(std::string_view, RegionData &)> const & getter)
{
  RegionData rd;

  rd.Clear();
  getter("Canada", rd);
  LangsBufferT langs;
  rd.GetLanguages(langs);
  TEST_EQUAL(langs[0], StringUtf8Multilang::GetLangIndex("en"), ());

  rd.Clear();
  getter("Canada_Quebec", rd);
  TEST(rd.IsSingleLanguage(StringUtf8Multilang::GetLangIndex("fr")), ());

  for (auto const region :
       {"Belize", "Canada", "Canada_Quebec", "US", "US_Guam", "US_Puerto Rico", "US_Florida_Gainesville",
        "United States Virgin Islands", "Liberia", "Federated States of Micronesia", "Marshall Islands", "Palau"})
  {
    rd.Clear();
    getter(region, rd);
    TEST_EQUAL(rd.Get(RegionData::RD_SPEED_LIMIT_SIGN_TYPE).size(), 1, (region));
    TEST(rd.GetSpeedLimitSignType() == SpeedLimitSignType::MUTCD, (region));
  }

  for (auto const region : {"Germany", "UK_Wales", "The Bahamas"})
  {
    rd.Clear();
    getter(region, rd);
    TEST(rd.Get(RegionData::RD_SPEED_LIMIT_SIGN_TYPE).empty(), (region));
    TEST(rd.GetSpeedLimitSignType() == SpeedLimitSignType::Vienna, (region));
  }

  for (auto uk : {"UK_England_Greater London", "UK_Wales"})
  {
    rd.Clear();
    getter(uk, rd);
    TEST_EQUAL(rd.Get(RegionData::RD_DRIVING), "l", ());

    /// @todo Add something like equal to GMT+0 check.
    auto const tz = om::tz::Deserialize(rd.Get(RegionData::RD_TIMEZONE));
    TEST(tz, ());
  }
}

UNIT_TEST(RegionData_Smoke)
{
  RunTestSamples([](std::string_view region, RegionData & data) { ReadRegionData(std::string(region), data); });
}

UNIT_TEST(RegionData_Map)
{
  AllRegionsData cont;
  ReadAllRegions(cont);

  RunTestSamples([&cont](std::string_view region, RegionData & data)
  {
    auto const * p = cont.Get(region);
    TEST(p, (region));
    data = *p;
  });
}

UNIT_TEST(RegionData_SpeedLimitSignType)
{
  RegionData parent;
  parent.SetSpeedLimitSignType(SpeedLimitSignType::MUTCD);

  RegionData child;
  TEST(child.GetSpeedLimitSignType() == SpeedLimitSignType::Vienna, ());
  child.MergeFrom(parent);
  TEST(child.GetSpeedLimitSignType() == SpeedLimitSignType::MUTCD, ());

  RegionData overriddenChild;
  overriddenChild.SetSpeedLimitSignType(SpeedLimitSignType::Vienna);
  overriddenChild.MergeFrom(parent);
  TEST(overriddenChild.GetSpeedLimitSignType() == SpeedLimitSignType::Vienna, ());

  child.Clear();
  TEST(child.GetSpeedLimitSignType() == SpeedLimitSignType::Vienna, ());
}

UNIT_TEST(RegionData_SpeedLimitSignTypeSerialization)
{
  std::optional<SpeedLimitSignType> const types[] = {std::nullopt, SpeedLimitSignType::Vienna,
                                                     SpeedLimitSignType::MUTCD};
  for (auto const type : types)
  {
    RegionData original;
    ReadRegionData("Germany", original);
    if (type)
      original.SetSpeedLimitSignType(*type);

    TEST_EQUAL(original.Get(RegionData::RD_SPEED_LIMIT_SIGN_TYPE).size(), type ? 1 : 0, ());

    std::vector<char> buffer;
    MemWriter writer(buffer);
    original.Serialize(writer);

    RegionData restored;
    MemReader reader(buffer.data(), buffer.size());
    ReaderSource source(reader);
    restored.Deserialize(source);

    TEST_EQUAL(restored.Get(RegionData::RD_SPEED_LIMIT_SIGN_TYPE), original.Get(RegionData::RD_SPEED_LIMIT_SIGN_TYPE),
               ());
    TEST(restored.GetSpeedLimitSignType() == original.GetSpeedLimitSignType(), ());
  }
}
