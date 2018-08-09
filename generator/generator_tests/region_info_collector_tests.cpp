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
auto const kOsmElementFull = MakeOsmElement(1, {{"place", "city"}, {"admin_level", "6"}});
}  // namespace

UNIT_TEST(RegionInfoCollector_Add)
{
  generator::RegionInfoCollector regionInfoCollector;
  regionInfoCollector.Add(kOsmElementFull);
  {
    auto const regionData = regionInfoCollector.Get(kOsmElementFull.id);
    TEST_EQUAL(regionData.m_osmId, kOsmElementFull.id, ());
    TEST_EQUAL(regionData.m_adminLevel, generator::AdminLevel::Six, ());
    TEST_EQUAL(regionData.m_place, generator::PlaceType::City, ());
  }

  regionInfoCollector.Add(kOsmElementEmpty);
  {
    auto const regionDataEmpty = regionInfoCollector.Get(kOsmElementEmpty.id);
    TEST_EQUAL(regionDataEmpty.m_osmId, kOsmElementEmpty.id, ());
    TEST_EQUAL(regionDataEmpty.m_adminLevel, generator::AdminLevel::Unknown, ());
    TEST_EQUAL(regionDataEmpty.m_place, generator::PlaceType::Unknown, ());
  }
}

UNIT_TEST(RegionInfoCollector_Get)
{
  generator::RegionInfoCollector regionInfoCollector;
  regionInfoCollector.Add(kOsmElementFull);

  auto const regionData = regionInfoCollector.Get(kOsmElementFull.id);
  TEST_EQUAL(regionData.m_osmId, kOsmElementFull.id, ());
  TEST_EQUAL(regionData.m_adminLevel, generator::AdminLevel::Six, ());
  TEST_EQUAL(regionData.m_place, generator::PlaceType::City, ());

  TEST_THROW(regionInfoCollector.Get(kNotExistingId), std::out_of_range, ());
}

UNIT_TEST(RegionInfoCollector_Exists)
{
  generator::RegionInfoCollector regionInfoCollector;
  regionInfoCollector.Add(kOsmElementFull);

  TEST(regionInfoCollector.Exists(kOsmElementFull.id), ());
  TEST(!regionInfoCollector.Exists(kNotExistingId), ());
}

UNIT_TEST(RegionInfoCollector_Save)
{
  generator::RegionInfoCollector regionInfoCollector;
  regionInfoCollector.Add(kOsmElementFull);
  auto const regionData = regionInfoCollector.Get(kOsmElementFull.id);

  auto & platform = GetPlatform();
  auto const tmpDir = platform.TmpDir();
  platform.SetWritableDirForTests(tmpDir);
  auto const name = my::JoinPath(tmpDir, "RegionInfoCollector.bin");
  regionInfoCollector.Save(name);
  {
    generator::RegionInfoCollector regionInfoCollector(name);
    auto const rRegionData = regionInfoCollector.Get(kOsmElementFull.id);

    TEST_EQUAL(regionData.m_osmId, rRegionData.m_osmId, ());
    TEST_EQUAL(regionData.m_adminLevel, rRegionData.m_adminLevel, ());
    TEST_EQUAL(regionData.m_place, rRegionData.m_place, ());
  }
}
