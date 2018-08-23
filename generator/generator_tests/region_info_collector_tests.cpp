#include "testing/testing.hpp"

#include "generator/osm_element.hpp"
#include "generator/region_info_collector.hpp"

#include "coding/file_name_utils.hpp"

#include "platform/platform.hpp"

#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace
{
using Tags = std::vector<std::pair<std::string, std::string>>;

OsmElement MakeOsmElement(uint64_t id, Tags const & tags)
{
  OsmElement el;
  el.id = id;
  for (const auto & t : tags)
    el.AddTag(t.first, t.second);

  return el;
}

auto const kNotExistingId = std::numeric_limits<uint64_t>::max();
auto const kOsmElementEmpty = MakeOsmElement(0, {});
auto const kOsmElementCity = MakeOsmElement(1, {{"place", "city"},
                                                {"admin_level", "6"}});
auto const kOsmElementCountry = MakeOsmElement(2, {{"admin_level", "2"},
                                                   {"ISO3166-1:alpha2", "RU"},
                                                   {"ISO3166-1:alpha3", "RUS"},
                                                   {"ISO3166-1:numeric", "643"}});
}  // namespace

UNIT_TEST(RegionInfoCollector_Add)
{
  generator::RegionInfoCollector regionInfoCollector;
  regionInfoCollector.Add(kOsmElementCity);
  {
    auto const regionData = regionInfoCollector.Get(kOsmElementCity.id);
    TEST_EQUAL(regionData.GetOsmId(), kOsmElementCity.id, ());
    TEST_EQUAL(regionData.GetAdminLevel(), generator::AdminLevel::Six, ());
    TEST_EQUAL(regionData.GetPlaceType(), generator::PlaceType::City, ());
    TEST(!regionData.HasIsoCodeAlpha2(), ());
    TEST(!regionData.HasIsoCodeAlpha3(), ());
    TEST(!regionData.HasIsoCodeAlphaNumeric(), ());
  }

  regionInfoCollector.Add(kOsmElementCountry);
  {
    auto const regionData = regionInfoCollector.Get(kOsmElementCountry.id);
    TEST_EQUAL(regionData.GetOsmId(), kOsmElementCountry.id, ());
    TEST_EQUAL(regionData.GetAdminLevel(), generator::AdminLevel::Two, ());
    TEST_EQUAL(regionData.GetPlaceType(), generator::PlaceType::Unknown, ());

    TEST(regionData.HasIsoCodeAlpha2(), ());
    TEST(regionData.HasIsoCodeAlpha3(), ());
    TEST(regionData.HasIsoCodeAlphaNumeric(), ());
    TEST_EQUAL(regionData.GetIsoCodeAlpha2(), "RU", ());
    TEST_EQUAL(regionData.GetIsoCodeAlpha3(), "RUS", ());
    TEST_EQUAL(regionData.GetIsoCodeAlphaNumeric(), "643", ());
  }

  regionInfoCollector.Add(kOsmElementEmpty);
  {
    auto const regionDataEmpty = regionInfoCollector.Get(kOsmElementEmpty.id);
    TEST_EQUAL(regionDataEmpty.GetOsmId(), kOsmElementEmpty.id, ());
    TEST_EQUAL(regionDataEmpty.GetAdminLevel(), generator::AdminLevel::Unknown, ());
    TEST_EQUAL(regionDataEmpty.GetPlaceType(), generator::PlaceType::Unknown, ());
    TEST(!regionDataEmpty.HasIsoCodeAlpha2(), ());
    TEST(!regionDataEmpty.HasIsoCodeAlpha3(), ());
    TEST(!regionDataEmpty.HasIsoCodeAlphaNumeric(), ());
  }
}

UNIT_TEST(RegionInfoCollector_Get)
{
  generator::RegionInfoCollector regionInfoCollector;
  regionInfoCollector.Add(kOsmElementCity);

  auto const regionData = regionInfoCollector.Get(kOsmElementCity.id);
  TEST_EQUAL(regionData.GetOsmId(), kOsmElementCity.id, ());
  TEST_EQUAL(regionData.GetAdminLevel(), generator::AdminLevel::Six, ());
  TEST_EQUAL(regionData.GetPlaceType(), generator::PlaceType::City, ());
}

UNIT_TEST(RegionInfoCollector_Exists)
{
  generator::RegionInfoCollector regionInfoCollector;
  regionInfoCollector.Add(kOsmElementCity);
  regionInfoCollector.Add(kOsmElementCountry);

  TEST(regionInfoCollector.Get(kOsmElementCountry.id).HasAdminLevel(), ());
  TEST(!regionInfoCollector.Get(kOsmElementCountry.id).HasPlaceType(), ());
  TEST(regionInfoCollector.Get(kOsmElementCountry.id).HasIsoCodeAlpha2(), ());
  TEST(regionInfoCollector.Get(kOsmElementCountry.id).HasIsoCodeAlpha3(), ());
  TEST(regionInfoCollector.Get(kOsmElementCountry.id).HasIsoCodeAlphaNumeric(), ());

  TEST(regionInfoCollector.Get(kOsmElementCity.id).HasAdminLevel(), ());
  TEST(regionInfoCollector.Get(kOsmElementCity.id).HasPlaceType(), ());
  TEST(!regionInfoCollector.Get(kOsmElementCity.id).HasIsoCodeAlpha2(), ());
  TEST(!regionInfoCollector.Get(kOsmElementCity.id).HasIsoCodeAlpha3(), ());
  TEST(!regionInfoCollector.Get(kOsmElementCity.id).HasIsoCodeAlphaNumeric(), ());

  TEST(!regionInfoCollector.Get(kNotExistingId).HasAdminLevel(), ());
  TEST(!regionInfoCollector.Get(kNotExistingId).HasPlaceType(), ());
  TEST(!regionInfoCollector.Get(kNotExistingId).HasIsoCodeAlpha2(), ());
  TEST(!regionInfoCollector.Get(kNotExistingId).HasIsoCodeAlpha3(), ());
  TEST(!regionInfoCollector.Get(kNotExistingId).HasIsoCodeAlphaNumeric(), ());
}

UNIT_TEST(RegionInfoCollector_Save)
{
  generator::RegionInfoCollector regionInfoCollector;
  regionInfoCollector.Add(kOsmElementCity);
  auto const regionCity = regionInfoCollector.Get(kOsmElementCity.id);
  regionInfoCollector.Add(kOsmElementCountry);
  auto const regionCountry = regionInfoCollector.Get(kOsmElementCountry.id);

  auto & platform = GetPlatform();
  auto const tmpDir = platform.TmpDir();
  platform.SetWritableDirForTests(tmpDir);
  auto const name = my::JoinPath(tmpDir, "RegionInfoCollector.bin");
  regionInfoCollector.Save(name);
  {
    generator::RegionInfoCollector regionInfoCollector(name);
    auto const rRegionData = regionInfoCollector.Get(kOsmElementCity.id);

    TEST_EQUAL(regionCity.GetOsmId(), rRegionData.GetOsmId(), ());
    TEST_EQUAL(regionCity.GetAdminLevel(), rRegionData.GetAdminLevel(), ());
    TEST_EQUAL(regionCity.GetPlaceType(), rRegionData.GetPlaceType(), ());
    TEST_EQUAL(regionCity.HasIsoCodeAlpha2(), rRegionData.HasIsoCodeAlpha2(), ());
    TEST_EQUAL(regionCity.HasIsoCodeAlpha3(), rRegionData.HasIsoCodeAlpha3(), ());
    TEST_EQUAL(regionCity.HasIsoCodeAlphaNumeric(), rRegionData.HasIsoCodeAlphaNumeric(), ());
  }

  {
    generator::RegionInfoCollector regionInfoCollector(name);
    auto const rRegionData = regionInfoCollector.Get(kOsmElementCountry.id);

    TEST_EQUAL(regionCountry.GetOsmId(), rRegionData.GetOsmId(), ());
    TEST_EQUAL(regionCountry.GetAdminLevel(), rRegionData.GetAdminLevel(), ());
    TEST_EQUAL(regionCountry.GetPlaceType(), rRegionData.GetPlaceType(), ());
    TEST_EQUAL(regionCountry.HasIsoCodeAlpha2(), rRegionData.HasIsoCodeAlpha2(), ());
    TEST_EQUAL(regionCountry.HasIsoCodeAlpha3(), rRegionData.HasIsoCodeAlpha3(), ());
    TEST_EQUAL(regionCountry.HasIsoCodeAlphaNumeric(), rRegionData.HasIsoCodeAlphaNumeric(), ());
    TEST_EQUAL(regionCountry.GetIsoCodeAlpha2(), rRegionData.GetIsoCodeAlpha2(), ());
    TEST_EQUAL(regionCountry.GetIsoCodeAlpha3(), rRegionData.GetIsoCodeAlpha3(), ());
    TEST_EQUAL(regionCountry.GetIsoCodeAlphaNumeric(), rRegionData.GetIsoCodeAlphaNumeric(), ());
  }
}
