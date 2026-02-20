#include "testing/testing.hpp"

#include "generator/region_meta.hpp"

#include "timezone/serdes.hpp"

#include <functional>

using namespace feature;

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
