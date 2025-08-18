#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "generator/city_roads_generator.hpp"

#include "routing/city_roads.hpp"

#include "indexer/data_source.hpp"

#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "defines.hpp"

namespace city_roads_tests
{
using namespace coding;
using namespace platform::tests_support;
using namespace platform;
using namespace routing;
using std::string, std::vector;

// Directory name for creating test mwm and temporary files.
string const kTestDir = "city_roads_generation_test";
// Temporary mwm name for testing.
string const kTestMwm = "test";

void BuildEmptyMwm(LocalCountryFile & country)
{
  generator::tests_support::TestMwmBuilder builder(country, feature::DataHeader::MapType::Country);
}

std::unique_ptr<CityRoads> LoadCityRoads(LocalCountryFile const & country)
{
  FrozenDataSource dataSource;
  auto const regResult = dataSource.RegisterMap(country);
  TEST_EQUAL(regResult.second, MwmSet::RegResult::Success, ());

  MwmSet::MwmHandle handle(dataSource.GetMwmHandleById(regResult.first));
  TEST(handle.IsAlive(), ());
  return routing::LoadCityRoads(handle);
}

/// \brief Builds mwm with city_roads section, read the section and compare original feature ids
/// and read ones.
/// \cityRoadFeatureIds a vector of feature ids which should be saved to city_roads
/// section and then read from it.
void TestCityRoadsBuilding(vector<uint32_t> && cityRoadFeatureIds)
{
  string const writableDir = GetPlatform().WritableDir();

  // Building empty mwm.
  LocalCountryFile country(base::JoinPath(writableDir, kTestDir), CountryFile(kTestMwm), 0 /* version */);
  ScopedDir const scopedDir(kTestDir);

  string const mwmRelativePath = base::JoinPath(kTestDir, kTestMwm + DATA_FILE_EXTENSION);
  ScopedFile const scopedMwm(mwmRelativePath, ScopedFile::Mode::Create);
  BuildEmptyMwm(country);

  // Adding city_roads section to mwm.
  string const mwmFullPath = base::JoinPath(writableDir, mwmRelativePath);
  vector<uint32_t> originalCityRoadFeatureIds = cityRoadFeatureIds;
  routing_builder::SerializeCityRoads(mwmFullPath, std::move(cityRoadFeatureIds));

  auto const cityRoads = LoadCityRoads(country);
  TEST(cityRoads, ());

  // Comparing loading form mwm and expected feature ids.
  if (originalCityRoadFeatureIds.empty())
  {
    TEST(!cityRoads->HaveCityRoads(), ());
    return;
  }

  TEST(cityRoads->HaveCityRoads(), ());

  sort(originalCityRoadFeatureIds.begin(), originalCityRoadFeatureIds.end());
  size_t const kMaxRoadFeatureId = originalCityRoadFeatureIds.back();
  CHECK_LESS(kMaxRoadFeatureId, std::numeric_limits<uint32_t>::max(), ());
  // Note. 2 is added below to test all the if-branches of CityRoads::IsCityRoad() method.
  for (uint32_t fid = 0; fid < kMaxRoadFeatureId + 2; ++fid)
  {
    bool const isCityRoad = binary_search(originalCityRoadFeatureIds.cbegin(), originalCityRoadFeatureIds.cend(), fid);
    TEST_EQUAL(cityRoads->IsCityRoad(fid), isCityRoad, (fid));
  }
}

UNIT_TEST(CityRoadsGenerationTest_Empty)
{
  TestCityRoadsBuilding(vector<uint32_t>({}));
}

UNIT_TEST(CityRoadsGenerationTest_FromZero)
{
  TestCityRoadsBuilding(vector<uint32_t>({0, 1, 10}));
}

UNIT_TEST(CityRoadsGenerationTest_CommonCase)
{
  TestCityRoadsBuilding(vector<uint32_t>({100, 203, 204, 1008, 1009}));
}

UNIT_TEST(CityRoadsGenerationTest_SortedIds1)
{
  TestCityRoadsBuilding(vector<uint32_t>({1000,  1203,  11004, 11008, 11009, 11010, 11011, 11012, 11013, 11014, 11015,
                                          11016, 11017, 11018, 11019, 11020, 11021, 11022, 11023, 11024, 11025}));
}

UNIT_TEST(CityRoadsGenerationTest_SortedIds2)
{
  TestCityRoadsBuilding(vector<uint32_t>({75000, 75001, 75004, 250000, 250001, 330003, 330007}));
}

UNIT_TEST(CityRoadsGenerationTest_UnsortedIds)
{
  TestCityRoadsBuilding(vector<uint32_t>({100, 1, 101, 2, 204, 1008, 1009}));
}

UNIT_TEST(CityRoadsGenerationTest_UnsortedIds2)
{
  TestCityRoadsBuilding(
      vector<uint32_t>({1000,  1203,  1,     11004, 11,    11009, 11010, 1011,  11012, 11013, 4, 11015,
                        11016, 11017, 11018, 11019, 11020, 11021, 11022, 11023, 11024, 11025, 2}));
}

UNIT_TEST(CityRoadsGenerationTest_UnsortedIds3)
{
  TestCityRoadsBuilding(vector<uint32_t>(
      {181998, 354195, 470394, 356217, 331537, 272789, 449031, 420305, 139273, 482371, 85866,  142591, 105206, 217360,
       380898, 390284, 96547,  110547, 201338, 428964, 246086, 29141,  179975, 493052, 53822,  238723, 316810, 349592,
       154010, 107966, 113307, 97285,  145351, 1153,   433747, 3176,   294890, 52537,  412384, 67264,  102626, 129329,
       49219,  289549, 68559,  364318, 211462, 170032, 59900,  257505, 164691, 75922,  209439, 308889, 143329, 140010,
       17175,  385681, 147374, 362296, 483109, 257221, 78957,  246540, 111001, 150408, 399397, 285220, 260539, 201792,
       378034, 349308, 68275,  278584, 14869,  71593,  34209,  146363, 177111, 319287, 25550,  39549,  130341, 225177,
       175089, 458144, 108978, 289265, 482825, 167725, 113023, 11551,  315969, 402715, 408055, 392033, 440099, 295901,
       228495, 297924, 89638,  214496, 207133, 362012, 397374, 424077, 343967, 84297,  230518, 159067, 6210,   331991,
       354649, 52253,  326650, 370671, 4187,   103637, 305287, 434759, 311923, 180429, 442122, 157044, 145067, 415419,
       237154, 404737, 269198, 308605, 57594,  310628, 418737, 359989, 36231,  7505,   226473, 436781, 173066, 97001,
       59617,  171771, 335309, 477484, 183747, 64957,  155749, 383375, 333286, 116341, 134386, 447463, 141022, 193133,
       271221, 169748, 474166, 60912,  66253,  50231,  98297,  321309, 386693, 456121, 247835, 372693, 365330, 20209,
       55571,  82275,  2165,   242495, 388715, 317264, 164407, 490188, 12846,  210451, 484847, 28868,  162385, 129045,
       463485, 92956,  337331, 338627, 100319, 182452, 303265, 73616,  262562, 62935,  294606, 466803, 215791, 468825,
       76934,  18187,  194429, 32913}));
}
}  // namespace city_roads_tests
