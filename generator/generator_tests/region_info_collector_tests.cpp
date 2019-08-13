#include "testing/testing.hpp"

#include "generator/feature_builder.hpp"
#include "generator/generator_tests/common.hpp"
#include "generator/osm_element.hpp"
#include "generator/regions/collector_region_info.hpp"
#include "generator/regions/region_info.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/geo_object_id.hpp"
#include "base/scope_guard.hpp"

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace generator_tests;
using namespace generator::regions;
using namespace feature;
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
FeatureBuilder const kEmptyFeature;
}  // namespace

UNIT_TEST(RegionInfoCollector_Collect)
{
  auto const filename = GetFileName();
  CollectorRegionInfo regionInfoCollector(filename);
  regionInfoCollector.Collect(kOsmElementCity);
  regionInfoCollector.Collect(kOsmElementCountry);
  regionInfoCollector.Collect(kOsmElementEmpty);
  regionInfoCollector.Save();

  RegionInfo regionInfo(filename);
  {
    auto const regionData = regionInfo.Get(MakeOsmRelation(kOsmElementCity.m_id));
    TEST_EQUAL(regionData.GetOsmId(), MakeOsmRelation(kOsmElementCity.m_id), ());
    TEST_EQUAL(regionData.GetAdminLevel(), AdminLevel::Six, ());
    TEST_EQUAL(regionData.GetPlaceType(), PlaceType::City, ());
    TEST(!regionData.GetIsoCodeAlpha2(), ());
    TEST(!regionData.GetIsoCodeAlpha3(), ());
    TEST(!regionData.GetIsoCodeAlphaNumeric(), ());
  }

  {
    auto const regionData = regionInfo.Get(MakeOsmRelation(kOsmElementCountry.m_id));
    TEST_EQUAL(regionData.GetOsmId(), MakeOsmRelation(kOsmElementCountry.m_id), ());
    TEST_EQUAL(regionData.GetAdminLevel(), AdminLevel::Two, ());
    TEST_EQUAL(regionData.GetPlaceType(), PlaceType::Unknown, ());

    TEST(regionData.GetIsoCodeAlpha2(), ());
    TEST(regionData.GetIsoCodeAlpha3(), ());
    TEST(regionData.GetIsoCodeAlphaNumeric(), ());
    TEST_EQUAL(*regionData.GetIsoCodeAlpha2(), "RU", ());
    TEST_EQUAL(*regionData.GetIsoCodeAlpha3(), "RUS", ());
    TEST_EQUAL(*regionData.GetIsoCodeAlphaNumeric(), "643", ());
  }

  {
    auto const regionDataEmpty = regionInfo.Get(MakeOsmRelation(kOsmElementEmpty.m_id));
    TEST_EQUAL(regionDataEmpty.GetOsmId(), MakeOsmRelation(kOsmElementEmpty.m_id), ());
    TEST_EQUAL(regionDataEmpty.GetAdminLevel(), AdminLevel::Unknown, ());
    TEST_EQUAL(regionDataEmpty.GetPlaceType(), PlaceType::Unknown, ());
    TEST(!regionDataEmpty.GetIsoCodeAlpha2(), ());
    TEST(!regionDataEmpty.GetIsoCodeAlpha3(), ());
    TEST(!regionDataEmpty.GetIsoCodeAlphaNumeric(), ());
  }
}

UNIT_TEST(RegionInfoCollector_Get)
{
  auto const filename = GetFileName();
  CollectorRegionInfo regionInfoCollector(filename);
  regionInfoCollector.Collect(kOsmElementCity);
  regionInfoCollector.Save();

  RegionInfo regionInfo(filename);
  auto const regionData = regionInfo.Get(MakeOsmRelation(kOsmElementCity.m_id));
  TEST_EQUAL(regionData.GetOsmId(), MakeOsmRelation(kOsmElementCity.m_id), ());
  TEST_EQUAL(regionData.GetAdminLevel(), AdminLevel::Six, ());
  TEST_EQUAL(regionData.GetPlaceType(), PlaceType::City, ());
}

UNIT_TEST(RegionInfoCollector_Exists)
{
  auto const filename = GetFileName();
  CollectorRegionInfo regionInfoCollector(filename);
  regionInfoCollector.Collect(kOsmElementCity);
  regionInfoCollector.Collect(kOsmElementCountry);
  regionInfoCollector.Save();

  RegionInfo regionInfo(filename);
  {
    auto const rg = regionInfo.Get(MakeOsmRelation(kOsmElementCountry.m_id));
    TEST_NOT_EQUAL(rg.GetAdminLevel(), AdminLevel::Unknown, ());
    TEST_EQUAL(rg.GetPlaceType(), PlaceType::Unknown, ());
    TEST(rg.GetIsoCodeAlpha2(), ());
    TEST(rg.GetIsoCodeAlpha3(), ());
    TEST(rg.GetIsoCodeAlphaNumeric(), ());
  }

  {
    auto const rg = regionInfo.Get(MakeOsmRelation(kOsmElementCity.m_id));
    TEST_NOT_EQUAL(rg.GetAdminLevel(), AdminLevel::Unknown, ());
    TEST_NOT_EQUAL(rg.GetPlaceType(), PlaceType::Unknown, ());
    TEST(!rg.GetIsoCodeAlpha2(), ());
    TEST(!rg.GetIsoCodeAlpha3(), ());
    TEST(!rg.GetIsoCodeAlphaNumeric(), ());
  }

  {
    auto const rg = regionInfo.Get(MakeOsmRelation(kNotExistingId));
    TEST_EQUAL(rg.GetAdminLevel(), AdminLevel::Unknown, ());
    TEST_EQUAL(rg.GetPlaceType(), PlaceType::Unknown, ());
    TEST(!rg.GetIsoCodeAlpha2(), ());
    TEST(!rg.GetIsoCodeAlpha3(), ());
    TEST(!rg.GetIsoCodeAlphaNumeric(), ());
  }
}

UNIT_TEST(RegionInfoCollector_MergeAndSave)
{
  auto const filename = generator_tests::GetFileName();
  SCOPE_GUARD(_, bind(Platform::RemoveFileIfExists, cref(filename)));

  auto c1 = std::make_shared<CollectorRegionInfo>(filename);
  auto c2 = c1->Clone();
  c1->Collect(kOsmElementCity);
  c2->Collect(kOsmElementCountry);
  c1->Finish();
  c2->Finish();
  c1->Merge(*c2);
  c1->Save();

 RegionInfo regionInfo(filename);
 {
   auto const rg = regionInfo.Get(MakeOsmRelation(kOsmElementCountry.m_id));
   TEST_NOT_EQUAL(rg.GetAdminLevel(), AdminLevel::Unknown, ());
   TEST_EQUAL(rg.GetPlaceType(), PlaceType::Unknown, ());
   TEST(rg.GetIsoCodeAlpha2(), ());
   TEST(rg.GetIsoCodeAlpha3(), ());
   TEST(rg.GetIsoCodeAlphaNumeric(), ());
 }

 {
   auto const rg = regionInfo.Get(MakeOsmRelation(kOsmElementCity.m_id));
   TEST_NOT_EQUAL(rg.GetAdminLevel(), AdminLevel::Unknown, ());
   TEST_NOT_EQUAL(rg.GetPlaceType(), PlaceType::Unknown, ());
   TEST(!rg.GetIsoCodeAlpha2(), ());
   TEST(!rg.GetIsoCodeAlpha3(), ());
   TEST(!rg.GetIsoCodeAlphaNumeric(), ());
 }
}
