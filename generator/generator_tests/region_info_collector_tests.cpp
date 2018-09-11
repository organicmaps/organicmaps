#include "testing/testing.hpp"

#include "generator/osm_element.hpp"
#include "generator/region_info_collector.hpp"

#include "coding/file_name_utils.hpp"

#include "base/geo_object_id.hpp"

#include "platform/platform.hpp"

#include <cstdint>
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
base::GeoObjectId CastId(uint64_t id)
{
  return base::MakeOsmRelation(id);
}
}  // namespace

UNIT_TEST(RegionInfoCollector_Add)
{
  generator::RegionInfoCollector regionInfoCollector;
  regionInfoCollector.Add(CastId(kOsmElementCity.id), kOsmElementCity);
  {
    auto const regionData = regionInfoCollector.Get(CastId(kOsmElementCity.id));
    TEST_EQUAL(regionData.GetOsmId(), CastId(kOsmElementCity.id), ());
    TEST_EQUAL(regionData.GetAdminLevel(), generator::AdminLevel::Six, ());
    TEST_EQUAL(regionData.GetPlaceType(), generator::PlaceType::City, ());
    TEST(!regionData.HasIsoCodeAlpha2(), ());
    TEST(!regionData.HasIsoCodeAlpha3(), ());
    TEST(!regionData.HasIsoCodeAlphaNumeric(), ());
  }

  regionInfoCollector.Add(CastId(kOsmElementCountry.id), kOsmElementCountry);
  {
    auto const regionData = regionInfoCollector.Get(CastId(kOsmElementCountry.id));
    TEST_EQUAL(regionData.GetOsmId(), CastId(kOsmElementCountry.id), ());
    TEST_EQUAL(regionData.GetAdminLevel(), generator::AdminLevel::Two, ());
    TEST_EQUAL(regionData.GetPlaceType(), generator::PlaceType::Unknown, ());

    TEST(regionData.HasIsoCodeAlpha2(), ());
    TEST(regionData.HasIsoCodeAlpha3(), ());
    TEST(regionData.HasIsoCodeAlphaNumeric(), ());
    TEST_EQUAL(regionData.GetIsoCodeAlpha2(), "RU", ());
    TEST_EQUAL(regionData.GetIsoCodeAlpha3(), "RUS", ());
    TEST_EQUAL(regionData.GetIsoCodeAlphaNumeric(), "643", ());
  }

  regionInfoCollector.Add(CastId(kOsmElementEmpty.id), kOsmElementEmpty);
  {
    auto const regionDataEmpty = regionInfoCollector.Get(CastId(kOsmElementEmpty.id));
    TEST_EQUAL(regionDataEmpty.GetOsmId(), CastId(kOsmElementEmpty.id), ());
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
  regionInfoCollector.Add(CastId(kOsmElementCity.id), kOsmElementCity);

  auto const regionData = regionInfoCollector.Get(CastId(kOsmElementCity.id));
  TEST_EQUAL(regionData.GetOsmId(), CastId(kOsmElementCity.id), ());
  TEST_EQUAL(regionData.GetAdminLevel(), generator::AdminLevel::Six, ());
  TEST_EQUAL(regionData.GetPlaceType(), generator::PlaceType::City, ());
}

UNIT_TEST(RegionInfoCollector_Exists)
{
  generator::RegionInfoCollector regionInfoCollector;
  regionInfoCollector.Add(CastId(kOsmElementCity.id), kOsmElementCity);
  regionInfoCollector.Add(CastId(kOsmElementCountry.id), kOsmElementCountry);

  {
    auto const rg = regionInfoCollector.Get(CastId(kOsmElementCountry.id));
    TEST(rg.HasAdminLevel(), ());
    TEST(!rg.HasPlaceType(), ());
    TEST(rg.HasIsoCodeAlpha2(), ());
    TEST(rg.HasIsoCodeAlpha3(), ());
    TEST(rg.HasIsoCodeAlphaNumeric(), ());
  }

  {
    auto const rg = regionInfoCollector.Get(CastId(kOsmElementCity.id));
    TEST(rg.HasAdminLevel(), ());
    TEST(rg.HasPlaceType(), ());
    TEST(!rg.HasIsoCodeAlpha2(), ());
    TEST(!rg.HasIsoCodeAlpha3(), ());
    TEST(!rg.HasIsoCodeAlphaNumeric(), ());
  }

  {
    auto const rg = regionInfoCollector.Get(CastId(kNotExistingId));
    TEST(!rg.HasAdminLevel(), ());
    TEST(!rg.HasPlaceType(), ());
    TEST(!rg.HasIsoCodeAlpha2(), ());
    TEST(!rg.HasIsoCodeAlpha3(), ());
    TEST(!rg.HasIsoCodeAlphaNumeric(), ());
  }
}

UNIT_TEST(RegionInfoCollector_Save)
{
  generator::RegionInfoCollector regionInfoCollector;
  regionInfoCollector.Add(CastId(kOsmElementCity.id), kOsmElementCity);
  auto const regionCity = regionInfoCollector.Get(CastId(kOsmElementCity.id));
  regionInfoCollector.Add(CastId(kOsmElementCountry.id), kOsmElementCountry);
  auto const regionCountry = regionInfoCollector.Get(CastId(kOsmElementCountry.id));

  auto & platform = GetPlatform();
  auto const tmpDir = platform.TmpDir();
  platform.SetWritableDirForTests(tmpDir);
  auto const name = my::JoinPath(tmpDir, "RegionInfoCollector.bin");
  regionInfoCollector.Save(name);
  {
    generator::RegionInfoCollector regionInfoCollector(name);
    auto const rRegionData = regionInfoCollector.Get(CastId(kOsmElementCity.id));

    TEST_EQUAL(regionCity.GetOsmId(), rRegionData.GetOsmId(), ());
    TEST_EQUAL(regionCity.GetAdminLevel(), rRegionData.GetAdminLevel(), ());
    TEST_EQUAL(regionCity.GetPlaceType(), rRegionData.GetPlaceType(), ());
    TEST_EQUAL(regionCity.HasIsoCodeAlpha2(), rRegionData.HasIsoCodeAlpha2(), ());
    TEST_EQUAL(regionCity.HasIsoCodeAlpha3(), rRegionData.HasIsoCodeAlpha3(), ());
    TEST_EQUAL(regionCity.HasIsoCodeAlphaNumeric(), rRegionData.HasIsoCodeAlphaNumeric(), ());
  }

  {
    generator::RegionInfoCollector regionInfoCollector(name);
    auto const rRegionData = regionInfoCollector.Get(CastId(kOsmElementCountry.id));

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
