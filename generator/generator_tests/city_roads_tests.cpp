#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "generator/city_roads_generator.hpp"

#include "routing/city_roads_serialization.hpp"

#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/platform.hpp"

#include "coding/compressed_bit_vector.hpp"
#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"
#include "coding/reader.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "defines.hpp"

using namespace coding;
using namespace platform::tests_support;
using namespace platform;
using namespace routing;
using namespace std;

namespace
{
// Directory name for creating test mwm and temporary files.
string const kTestDir = "city_roads_generation_test";
// Temporary mwm name for testing.
string const kTestMwm = "test";

void BuildEmptyMwm(LocalCountryFile & country)
{
  generator::tests_support::TestMwmBuilder builder(country, feature::DataHeader::country);
}

unique_ptr<CompressedBitVector> LoadCityRoads(string const & mwmFilePath)
{
  FilesContainerR const cont(mwmFilePath);
  if (!cont.IsExist(CITY_ROADS_FILE_TAG))
    return nullptr;

  try
  {
    FilesContainerR::TReader const reader = cont.GetReader(CITY_ROADS_FILE_TAG);
    ReaderSource<FilesContainerR::TReader> src(reader);

    CityRoadsHeader header;
    header.Deserialize(src);
    TEST_EQUAL(header.m_version, 0, ());

    return CompressedBitVectorBuilder::DeserializeFromSource(src);
  }
  catch (Reader::OpenException const & e)
  {
    TEST(false, ("Error while reading", CITY_ROADS_FILE_TAG, "section.", e.Msg()));
    return nullptr;
  }
}

/// \brief Builds mwm with city_roads section, read the section and compare original feature ids
/// and read ones.
/// \cityRoadFeatureIds a vector of feature ids which should be saved to city_roads
/// section and then read from it.
void TestCityRoadsBuilding(vector<uint64_t> && cityRoadFeatureIds)
{
  string const writableDir = GetPlatform().WritableDir();

  // Building empty mwm.
  LocalCountryFile country(my::JoinPath(writableDir, kTestDir), CountryFile(kTestMwm),
                           0 /* version */);
  ScopedDir const scopedDir(kTestDir);
  string const mwmRelativePath = my::JoinPath(kTestDir, kTestMwm + DATA_FILE_EXTENSION);
  ScopedFile const scopedMwm(mwmRelativePath, ScopedFile::Mode::Create);
  BuildEmptyMwm(country);

  // Adding city_roads section to mwm.
  string const mwmFullPath = my::JoinPath(writableDir, mwmRelativePath);
  vector<uint64_t> originalCityRoadFeatureIds = cityRoadFeatureIds;
  SerializeCityRoads(mwmFullPath, move(cityRoadFeatureIds));

  // Loading city_roads section.
  auto const loadedCityRoadFeatureIds = LoadCityRoads(mwmFullPath);

  // Comparing loading form mwm and expected feature ids.
  if (originalCityRoadFeatureIds.empty())
  {
    TEST(!loadedCityRoadFeatureIds, ());
    return;
  }

  TEST(loadedCityRoadFeatureIds, ());
  sort(originalCityRoadFeatureIds.begin(), originalCityRoadFeatureIds.end());
  size_t const kMaxRoadFeatureId = originalCityRoadFeatureIds.back();
  for (uint64_t fid = 0; fid < kMaxRoadFeatureId; ++fid)
  {
    bool const isCityRoad =
        binary_search(originalCityRoadFeatureIds.cbegin(), originalCityRoadFeatureIds.cend(), fid);
    TEST_EQUAL(loadedCityRoadFeatureIds->GetBit(fid), isCityRoad, (fid));
  }
}

UNIT_TEST(CityRoadsGenerationTest_Empty)
{
  TestCityRoadsBuilding(vector<uint64_t>({}));
}

UNIT_TEST(CityRoadsGenerationTest_FromZero)
{
  TestCityRoadsBuilding(vector<uint64_t>({0, 1, 10}));
}

UNIT_TEST(CityRoadsGenerationTest_CommonCase)
{
  TestCityRoadsBuilding(vector<uint64_t>({100, 203, 204, 1008, 1009}));
}

UNIT_TEST(CityRoadsGenerationTest_BigNumbers)
{
  TestCityRoadsBuilding(
      vector<uint64_t>({1000,  1203,  11004,  11008, 11009, 11010, 11011, 11012, 11013, 11014, 11015,
                        11016, 11017, 11018, 11019, 11020, 11021, 11022, 11023, 11024, 11025}));
}

UNIT_TEST(CityRoadsGenerationTest_UnsortedIds)
{
  TestCityRoadsBuilding(vector<uint64_t>({100, 1, 101, 2, 204, 1008, 1009}));
}

UNIT_TEST(CityRoadsGenerationTest_UnsortedIds2)
{
  TestCityRoadsBuilding(
      vector<uint64_t>({1000,  1203, 1, 11004,  11, 11009, 11010, 1011, 11012, 11013, 11, 4, 11015,
                        11016, 11017, 11018, 11019, 11020, 11021, 11022, 11023, 11024, 11025, 2}));
}
}  // namespace
