#include "testing/testing.hpp"

#include "generator/generator_tests_support/routing_helpers.hpp"
#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "generator/road_access_generator.hpp"

#include "routing/road_access_serialization.hpp"

#include "platform/country_file.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"

#include "base/logging.hpp"

#include <string>

using namespace feature;
using namespace generator;
using namespace platform::tests_support;
using namespace platform;
using namespace routing;
using std::string;

namespace
{
string const kTestDir = "road_access_generation_test";
string const kTestMwm = "test";
string const kRoadAccessFilename = "road_access_in_osm_ids.csv";
string const kOsmIdsToFeatureIdsName = "osm_ids_to_feature_ids" OSM2FEATURE_FILE_EXTENSION;

void BuildEmptyMwm(LocalCountryFile & country)
{
  generator::tests_support::TestMwmBuilder builder(country, feature::DataHeader::country);
}

void LoadRoadAccess(string const & mwmFilePath, RoadAccess & roadAccess)
{
  FilesContainerR const cont(mwmFilePath);
  TEST(cont.IsExist(ROAD_ACCESS_FILE_TAG), ());

  try
  {
    FilesContainerR::TReader const reader = cont.GetReader(ROAD_ACCESS_FILE_TAG);
    ReaderSource<FilesContainerR::TReader> src(reader);

    RoadAccessSerializer::Deserialize(src, roadAccess);
  }
  catch (Reader::OpenException const & e)
  {
    TEST(false, ("Error while reading", ROAD_ACCESS_FILE_TAG, "section.", e.Msg()));
  }
}

// todo(@m) This helper function is almost identical to the one in restriction_test.cpp.
void TestRoadAccess(string const & roadAccessContent, string const & mappingContent)
{
  Platform & platform = GetPlatform();
  string const writableDir = platform.WritableDir();

  // Building empty mwm.
  LocalCountryFile country(my::JoinPath(writableDir, kTestDir), CountryFile(kTestMwm),
                           0 /* version */);
  ScopedDir const scopedDir(kTestDir);
  string const mwmRelativePath = my::JoinPath(kTestDir, kTestMwm + DATA_FILE_EXTENSION);
  ScopedFile const scopedMwm(mwmRelativePath);
  BuildEmptyMwm(country);

  // Creating a file with road access.
  string const roadAccessRelativePath = my::JoinPath(kTestDir, kRoadAccessFilename);
  ScopedFile const roadAccessFile(roadAccessRelativePath, roadAccessContent);

  // Creating osm ids to feature ids mapping.
  string const mappingRelativePath = my::JoinPath(kTestDir, kOsmIdsToFeatureIdsName);
  ScopedFile const mappingFile(mappingRelativePath);
  string const mappingFullPath = mappingFile.GetFullPath();
  ReEncodeOsmIdsToFeatureIdsMapping(mappingContent, mappingFullPath);

  // Adding road access section to mwm.
  string const roadAccessFullPath = my::JoinPath(writableDir, roadAccessRelativePath);
  string const mwmFullPath = my::JoinPath(writableDir, mwmRelativePath);
  BuildRoadAccessInfo(mwmFullPath, roadAccessFullPath, mappingFullPath);

  // Reading from mwm section and testing road access.
  RoadAccess roadAccessFromMwm;
  LoadRoadAccess(mwmFullPath, roadAccessFromMwm);
  RoadAccessCollector const collector(roadAccessFullPath, mappingFullPath);
  TEST(collector.IsValid(), ());
  TEST_EQUAL(roadAccessFromMwm, collector.GetRoadAccess(), ());
}

UNIT_TEST(RoadAccess_Smoke)
{
  string const roadAccessContent = "";
  string const osmIdsToFeatureIdsContent = "";
  TestRoadAccess(roadAccessContent, osmIdsToFeatureIdsContent);
}

UNIT_TEST(RoadAccess_AccessPrivate)
{
  string const roadAccessContent = R"(access=private 0)";
  string const osmIdsToFeatureIdsContent = R"(0, 0,)";
  TestRoadAccess(roadAccessContent, osmIdsToFeatureIdsContent);
}

UNIT_TEST(RoadAccess_Access_And_Barriers)
{
  string const roadAccessContent = R"(access=private 10
                                     access=private 20
                                     barrier=gate 30
                                     barrier=gate 40)";
  string const osmIdsToFeatureIdsContent = R"(10, 1,
                                             20, 2,
                                             30, 3,
                                             40, 4,)";
  TestRoadAccess(roadAccessContent, osmIdsToFeatureIdsContent);
}
}  // namespace
