#include "testing/testing.hpp"

#include "generator/generator_tests/regions_common.hpp"
#include "generator/osm_element.hpp"
#include "generator/regions/collector_region_info.hpp"

#include "coding/file_name_utils.hpp"

#include "base/geo_object_id.hpp"

#include "platform/platform.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

using namespace generator::regions;
using namespace base;

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

UNIT_TEST(RegionInfoCollector_Collect)
{
  CollectorRegionInfo regionInfoCollector(GetFileName());
  regionInfoCollector.Collect(MakeOsmRelation(kOsmElementCity.id), kOsmElementCity);
  regionInfoCollector.Collect(MakeOsmRelation(kOsmElementCountry.id), kOsmElementCountry);
  regionInfoCollector.Collect(MakeOsmRelation(kOsmElementEmpty.id), kOsmElementEmpty);
  regionInfoCollector.Save();

  RegionInfo regionInfo(GetFileName());
  {
    auto const regionData = regionInfo.Get(MakeOsmRelation(kOsmElementCity.id));
    TEST_EQUAL(regionData.GetOsmId(), MakeOsmRelation(kOsmElementCity.id), ());
    TEST_EQUAL(regionData.GetAdminLevel(), AdminLevel::Six, ());
    TEST_EQUAL(regionData.GetPlaceType(), PlaceType::City, ());
    TEST(!regionData.HasIsoCodeAlpha2(), ());
    TEST(!regionData.HasIsoCodeAlpha3(), ());
    TEST(!regionData.HasIsoCodeAlphaNumeric(), ());
  }

  {
    auto const regionData = regionInfo.Get(MakeOsmRelation(kOsmElementCountry.id));
    TEST_EQUAL(regionData.GetOsmId(), MakeOsmRelation(kOsmElementCountry.id), ());
    TEST_EQUAL(regionData.GetAdminLevel(), AdminLevel::Two, ());
    TEST_EQUAL(regionData.GetPlaceType(), PlaceType::Unknown, ());

    TEST(regionData.HasIsoCodeAlpha2(), ());
    TEST(regionData.HasIsoCodeAlpha3(), ());
    TEST(regionData.HasIsoCodeAlphaNumeric(), ());
    TEST_EQUAL(regionData.GetIsoCodeAlpha2(), "RU", ());
    TEST_EQUAL(regionData.GetIsoCodeAlpha3(), "RUS", ());
    TEST_EQUAL(regionData.GetIsoCodeAlphaNumeric(), "643", ());
  }

  {
    auto const regionDataEmpty = regionInfo.Get(MakeOsmRelation(kOsmElementEmpty.id));
    TEST_EQUAL(regionDataEmpty.GetOsmId(), MakeOsmRelation(kOsmElementEmpty.id), ());
    TEST_EQUAL(regionDataEmpty.GetAdminLevel(), AdminLevel::Unknown, ());
    TEST_EQUAL(regionDataEmpty.GetPlaceType(), PlaceType::Unknown, ());
    TEST(!regionDataEmpty.HasIsoCodeAlpha2(), ());
    TEST(!regionDataEmpty.HasIsoCodeAlpha3(), ());
    TEST(!regionDataEmpty.HasIsoCodeAlphaNumeric(), ());
  }
}

UNIT_TEST(RegionInfoCollector_Get)
{
  CollectorRegionInfo regionInfoCollector(GetFileName());
  regionInfoCollector.Collect(MakeOsmRelation(kOsmElementCity.id), kOsmElementCity);
  regionInfoCollector.Save();

  RegionInfo regionInfo(GetFileName());
  auto const regionData = regionInfo.Get(MakeOsmRelation(kOsmElementCity.id));
  TEST_EQUAL(regionData.GetOsmId(), MakeOsmRelation(kOsmElementCity.id), ());
  TEST_EQUAL(regionData.GetAdminLevel(), AdminLevel::Six, ());
  TEST_EQUAL(regionData.GetPlaceType(), PlaceType::City, ());
}

UNIT_TEST(RegionInfoCollector_Exists)
{
  CollectorRegionInfo regionInfoCollector(GetFileName());
  regionInfoCollector.Collect(MakeOsmRelation(kOsmElementCity.id), kOsmElementCity);
  regionInfoCollector.Collect(MakeOsmRelation(kOsmElementCountry.id), kOsmElementCountry);
  regionInfoCollector.Save();

  RegionInfo regionInfo(GetFileName());
  {
    auto const rg = regionInfo.Get(MakeOsmRelation(kOsmElementCountry.id));
    TEST(rg.HasAdminLevel(), ());
    TEST(!rg.HasPlaceType(), ());
    TEST(rg.HasIsoCodeAlpha2(), ());
    TEST(rg.HasIsoCodeAlpha3(), ());
    TEST(rg.HasIsoCodeAlphaNumeric(), ());
  }

  {
    auto const rg = regionInfo.Get(MakeOsmRelation(kOsmElementCity.id));
    TEST(rg.HasAdminLevel(), ());
    TEST(rg.HasPlaceType(), ());
    TEST(!rg.HasIsoCodeAlpha2(), ());
    TEST(!rg.HasIsoCodeAlpha3(), ());
    TEST(!rg.HasIsoCodeAlphaNumeric(), ());
  }

  {
    auto const rg = regionInfo.Get(MakeOsmRelation(kNotExistingId));
    TEST(!rg.HasAdminLevel(), ());
    TEST(!rg.HasPlaceType(), ());
    TEST(!rg.HasIsoCodeAlpha2(), ());
    TEST(!rg.HasIsoCodeAlpha3(), ());
    TEST(!rg.HasIsoCodeAlphaNumeric(), ());
  }
}
