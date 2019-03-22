#include "testing/testing.hpp"

#include "generator/feature_builder.hpp"
#include "generator/generator_tests/common.hpp"
#include "generator/osm_element.hpp"
#include "generator/regions/collector_region_info.hpp"
#include "generator/regions/region_info.hpp"

#include "coding/file_name_utils.hpp"

#include "base/geo_object_id.hpp"

#include "platform/platform.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

using namespace generator_tests;
using namespace generator::regions;
using namespace base;

namespace
{
auto const kNotExistingId = std::numeric_limits<uint64_t>::max();
auto const kOsmElementEmpty = MakeOsmElement(0, {}, OsmElement::EntityType::Relation);
auto const kOsmElementCity = MakeOsmElement(1, {{"place", "city"},
                                                {"admin_level", "6"}},
                                            OsmElement::EntityType::Relation);
auto const kOsmElementCountry = MakeOsmElement(2, {{"admin_level", "2"},
                                                   {"ISO3166-1:alpha2", "RU"},
                                                   {"ISO3166-1:alpha3", "RUS"},
                                                   {"ISO3166-1:numeric", "643"}},
                                               OsmElement::EntityType::Relation);
FeatureBuilder1 const kEmptyFeature;
}  // namespace

UNIT_TEST(RegionInfoCollector_Collect)
{
  auto const filename = GetFileName();
  CollectorRegionInfo regionInfoCollector(filename);
  regionInfoCollector.CollectFeature(kEmptyFeature, kOsmElementCity);
  regionInfoCollector.CollectFeature(kEmptyFeature, kOsmElementCountry);
  regionInfoCollector.CollectFeature(kEmptyFeature, kOsmElementEmpty);
  regionInfoCollector.Save();

  RegionInfo regionInfo(filename);
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
  auto const filename = GetFileName();
  CollectorRegionInfo regionInfoCollector(filename);
  regionInfoCollector.CollectFeature(kEmptyFeature, kOsmElementCity);
  regionInfoCollector.Save();

  RegionInfo regionInfo(filename);
  auto const regionData = regionInfo.Get(MakeOsmRelation(kOsmElementCity.id));
  TEST_EQUAL(regionData.GetOsmId(), MakeOsmRelation(kOsmElementCity.id), ());
  TEST_EQUAL(regionData.GetAdminLevel(), AdminLevel::Six, ());
  TEST_EQUAL(regionData.GetPlaceType(), PlaceType::City, ());
}

UNIT_TEST(RegionInfoCollector_Exists)
{
  auto const filename = GetFileName();
  CollectorRegionInfo regionInfoCollector(filename);
  regionInfoCollector.CollectFeature(kEmptyFeature, kOsmElementCity);
  regionInfoCollector.CollectFeature(kEmptyFeature, kOsmElementCountry);
  regionInfoCollector.Save();

  RegionInfo regionInfo(filename);
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
