#include "testing/testing.hpp"

#include "generator/generator_tests_support/routing_helpers.hpp"
#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "generator/road_access_generator.hpp"

#include "routing/road_access_serialization.hpp"
#include "routing/segment.hpp"

#include "platform/country_file.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include "indexer/classificator_loader.hpp"

#include "geometry/point2d.hpp"

#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <map>
#include <string>
#include <vector>

using namespace std;

using namespace feature;
using namespace generator::tests_support;
using namespace generator;
using namespace platform::tests_support;
using namespace platform;
using namespace routing;
using namespace std;

namespace
{
string const kTestDir = "road_access_generation_test";
string const kTestMwm = "test";
string const kRoadAccessFilename = "road_access_in_osm_ids.csv";
string const kOsmIdsToFeatureIdsName = "osm_ids_to_feature_ids" OSM2FEATURE_FILE_EXTENSION;

void BuildTestMwmWithRoads(LocalCountryFile & country)
{
  generator::tests_support::TestMwmBuilder builder(country, feature::DataHeader::country);

  for (size_t i = 0; i < 10; ++i)
  {
    string const name = "road " + strings::to_string(i);
    string const lang = "en";
    vector<m2::PointD> points;
    for (size_t j = 0; j < 10; ++j)
      points.emplace_back(static_cast<double>(i), static_cast<double>(j));

    builder.Add(TestRoad(points, name, lang));
  }
}

void LoadRoadAccess(string const & mwmFilePath, VehicleType vehicleType, RoadAccess & roadAccess)
{
  FilesContainerR const cont(mwmFilePath);
  TEST(cont.IsExist(ROAD_ACCESS_FILE_TAG), ());

  try
  {
    FilesContainerR::TReader const reader = cont.GetReader(ROAD_ACCESS_FILE_TAG);
    ReaderSource<FilesContainerR::TReader> src(reader);

    RoadAccessSerializer::Deserialize(src, vehicleType, roadAccess);
  }
  catch (Reader::OpenException const & e)
  {
    TEST(false, ("Error while reading", ROAD_ACCESS_FILE_TAG, "section.", e.Msg()));
  }
}

// todo(@m) This helper function is almost identical to the one in restriction_test.cpp.
RoadAccessCollector::RoadAccessByVehicleType SaveAndLoadRoadAccess(string const & roadAccessContent,
                                                                   string const & mappingContent)
{
  classificator::Load();

  Platform & platform = GetPlatform();
  string const writableDir = platform.WritableDir();

  // Building empty mwm.
  LocalCountryFile country(my::JoinPath(writableDir, kTestDir), CountryFile(kTestMwm),
                           0 /* version */);
  ScopedDir const scopedDir(kTestDir);
  string const mwmRelativePath = my::JoinPath(kTestDir, kTestMwm + DATA_FILE_EXTENSION);
  ScopedFile const scopedMwm(mwmRelativePath, ScopedFile::Mode::Create);
  BuildTestMwmWithRoads(country);

  // Creating a file with road access.
  string const roadAccessRelativePath = my::JoinPath(kTestDir, kRoadAccessFilename);
  ScopedFile const roadAccessFile(roadAccessRelativePath, roadAccessContent);

  // Creating osm ids to feature ids mapping.
  string const mappingRelativePath = my::JoinPath(kTestDir, kOsmIdsToFeatureIdsName);
  ScopedFile const mappingFile(mappingRelativePath, ScopedFile::Mode::Create);
  string const mappingFullPath = mappingFile.GetFullPath();
  ReEncodeOsmIdsToFeatureIdsMapping(mappingContent, mappingFullPath);

  // Adding road access section to mwm.
  string const roadAccessFullPath = my::JoinPath(writableDir, roadAccessRelativePath);
  string const mwmFullPath = my::JoinPath(writableDir, mwmRelativePath);
  BuildRoadAccessInfo(mwmFullPath, roadAccessFullPath, mappingFullPath);

  // Reading from mwm section and testing road access.
  RoadAccessCollector::RoadAccessByVehicleType roadAccessFromMwm;
  for (size_t i = 0; i < static_cast<size_t>(VehicleType::Count); ++i)
  {
    auto const vehicleType = static_cast<VehicleType>(i);
    LoadRoadAccess(mwmFullPath, vehicleType, roadAccessFromMwm[i]);
  }
  RoadAccessCollector const collector(mwmFullPath, roadAccessFullPath, mappingFullPath);
  TEST(collector.IsValid(), ());
  TEST_EQUAL(roadAccessFromMwm, collector.GetRoadAccessAllTypes(), ());
  return roadAccessFromMwm;
}

UNIT_TEST(RoadAccess_Smoke)
{
  string const roadAccessContent = "";
  string const osmIdsToFeatureIdsContent = "";
  SaveAndLoadRoadAccess(roadAccessContent, osmIdsToFeatureIdsContent);
}

UNIT_TEST(RoadAccess_AccessPrivate)
{
  string const roadAccessContent = R"(Car Private 0)";
  string const osmIdsToFeatureIdsContent = R"(0, 0,)";
  auto const roadAccessAllTypes =
      SaveAndLoadRoadAccess(roadAccessContent, osmIdsToFeatureIdsContent);
  auto const carRoadAccess = roadAccessAllTypes[static_cast<size_t>(VehicleType::Car)];
  TEST_EQUAL(carRoadAccess.GetSegmentType(Segment(0, 0, 0, false)), RoadAccess::Type::Private, ());
}

UNIT_TEST(RoadAccess_Access_Multiple_Vehicle_Types)
{
  string const roadAccessContent = R"(Car Private 10
                                     Car Private 20
                                     Bicycle No 30
                                     Car Destination 40)";
  string const osmIdsToFeatureIdsContent = R"(10, 1,
                                             20, 2,
                                             30, 3,
                                             40, 4,)";
  auto const roadAccessAllTypes =
      SaveAndLoadRoadAccess(roadAccessContent, osmIdsToFeatureIdsContent);
  auto const carRoadAccess = roadAccessAllTypes[static_cast<size_t>(VehicleType::Car)];
  auto const bicycleRoadAccess = roadAccessAllTypes[static_cast<size_t>(VehicleType::Bicycle)];
  TEST_EQUAL(carRoadAccess.GetSegmentType(Segment(0, 1, 0, false)), RoadAccess::Type::Private, ());
  TEST_EQUAL(carRoadAccess.GetSegmentType(Segment(0, 2, 2, true)), RoadAccess::Type::Private, ());
  TEST_EQUAL(carRoadAccess.GetSegmentType(Segment(0, 3, 1, true)), RoadAccess::Type::Yes, ());
  TEST_EQUAL(carRoadAccess.GetSegmentType(Segment(0, 4, 3, false)), RoadAccess::Type::Destination,
             ());
  TEST_EQUAL(bicycleRoadAccess.GetSegmentType(Segment(0, 3, 0, false)), RoadAccess::Type::No, ());
}
}  // namespace
