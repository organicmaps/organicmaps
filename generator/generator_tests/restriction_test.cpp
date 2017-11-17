#include "testing/testing.hpp"

#include "generator/generator_tests_support/routing_helpers.hpp"
#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "generator/restriction_collector.hpp"
#include "generator/restriction_generator.hpp"

#include "routing/restriction_loader.hpp"

#include "platform/country_file.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"

#include "base/logging.hpp"

#include <string>

using namespace std;

using namespace feature;
using namespace generator;
using namespace platform::tests_support;
using namespace platform;
using namespace routing;

namespace
{
// Directory name for creating test mwm and temporary files.
string const kTestDir = "restriction_generation_test";
// Temporary mwm name for testing.
string const kTestMwm = "test";
string const kRestrictionFileName = "restrictions_in_osm_ids.csv";
string const kOsmIdsToFeatureIdsName = "osm_ids_to_feature_ids" OSM2FEATURE_FILE_EXTENSION;

void BuildEmptyMwm(LocalCountryFile & country)
{
  generator::tests_support::TestMwmBuilder builder(country, feature::DataHeader::country);
}

void LoadRestrictions(string const & mwmFilePath, RestrictionVec & restrictions)
{
  FilesContainerR const cont(mwmFilePath);
  if (!cont.IsExist(RESTRICTIONS_FILE_TAG))
    return;

  try
  {
    FilesContainerR::TReader const reader = cont.GetReader(RESTRICTIONS_FILE_TAG);
    ReaderSource<FilesContainerR::TReader> src(reader);
    RestrictionHeader header;
    header.Deserialize(src);

    RestrictionVec restrictionsOnly;
    RestrictionSerializer::Deserialize(header, restrictions, restrictionsOnly, src);
    restrictions.insert(restrictions.end(), restrictionsOnly.cbegin(), restrictionsOnly.cend());
  }
  catch (Reader::OpenException const & e)
  {
    TEST(false, ("Error while reading", ROAD_ACCESS_FILE_TAG, "section.", e.Msg()));
  }
}

/// \brief Generates a restriction section, adds it to an empty mwm,
/// loads the restriction section and test loaded restrictions.
/// \param restrictionContent comma separated text with restrictions in osm id terms.
/// \param mappingContent comma separated text with mapping from osm ids to feature ids.
void TestRestrictionBuilding(string const & restrictionContent, string const & mappingContent)
{
  Platform & platform = GetPlatform();
  string const writableDir = platform.WritableDir();

  // Building empty mwm.
  LocalCountryFile country(my::JoinPath(writableDir, kTestDir), CountryFile(kTestMwm),
                           0 /* version */);
  ScopedDir const scopedDir(kTestDir);
  string const mwmRelativePath = my::JoinPath(kTestDir, kTestMwm + DATA_FILE_EXTENSION);
  ScopedFile const scopedMwm(mwmRelativePath, ScopedFile::Mode::Create);
  BuildEmptyMwm(country);

  // Creating a file with restrictions.
  string const restrictionRelativePath = my::JoinPath(kTestDir, kRestrictionFileName);
  ScopedFile const restrictionScopedFile(restrictionRelativePath, restrictionContent);

  // Creating osm ids to feature ids mapping.
  string const mappingRelativePath = my::JoinPath(kTestDir, kOsmIdsToFeatureIdsName);
  ScopedFile const mappingFile(mappingRelativePath, ScopedFile::Mode::Create);
  string const mappingFullPath = mappingFile.GetFullPath();
  ReEncodeOsmIdsToFeatureIdsMapping(mappingContent, mappingFullPath);

  // Adding restriction section to mwm.
  string const restrictionFullPath = my::JoinPath(writableDir, restrictionRelativePath);
  string const mwmFullPath = my::JoinPath(writableDir, mwmRelativePath);
  BuildRoadRestrictions(mwmFullPath, restrictionFullPath, mappingFullPath);

  // Reading from mwm section and testing restrictions.
  RestrictionVec restrictionsFromMwm;
  LoadRestrictions(mwmFullPath, restrictionsFromMwm);
  RestrictionCollector const restrictionCollector(restrictionFullPath, mappingFullPath);

  TEST_EQUAL(restrictionsFromMwm, restrictionCollector.GetRestrictions(), ());
}

UNIT_TEST(RestrictionGenerationTest_NoRestriction)
{
  string const restrictionContent = "";
  string const osmIdsToFeatureIdsContent = "";
  TestRestrictionBuilding(restrictionContent, osmIdsToFeatureIdsContent);
}

UNIT_TEST(RestrictionGenerationTest_ZeroId)
{
  string const restrictionContent = R"(Only, 0, 0,)";
  string const osmIdsToFeatureIdsContent = R"(0, 0,)";
  TestRestrictionBuilding(restrictionContent, osmIdsToFeatureIdsContent);
}

UNIT_TEST(RestrictionGenerationTest_OneRestriction)
{
  string const restrictionContent = R"(No, 10, 10,)";
  string const osmIdsToFeatureIdsContent = R"(10, 1,)";
  TestRestrictionBuilding(restrictionContent, osmIdsToFeatureIdsContent);
}

UNIT_TEST(RestrictionGenerationTest_ThreeRestrictions)
{
  string const restrictionContent = R"(No, 10, 10,
                                       Only, 10, 20
                                       Only, 30, 40)";
  string const osmIdsToFeatureIdsContent = R"(10, 1,
                                              20, 2
                                              30, 3,
                                              40, 4)";
  TestRestrictionBuilding(restrictionContent, osmIdsToFeatureIdsContent);
}

UNIT_TEST(RestrictionGenerationTest_SevenRestrictions)
{
  string const restrictionContent = R"(No, 10, 10,
                                       No, 20, 20,
                                       Only, 10, 20,
                                       Only, 20, 30,
                                       No, 30, 30,
                                       No, 40, 40,
                                       Only, 30, 40,)";
  string const osmIdsToFeatureIdsContent = R"(10, 1,
                                              20, 2,
                                              30, 3,
                                              40, 4)";
  TestRestrictionBuilding(restrictionContent, osmIdsToFeatureIdsContent);
}

UNIT_TEST(RestrictionGenerationTest_ThereAndMoreLinkRestrictions)
{
  string const restrictionContent = R"(No, 10, 10,
                                       No, 20, 20,
                                       Only, 10, 20, 30, 40
                                       Only, 20, 30, 40)";
  string const osmIdsToFeatureIdsContent = R"(10, 1,
                                             20, 2,
                                             30, 3,
                                             40, 4)";
  TestRestrictionBuilding(restrictionContent, osmIdsToFeatureIdsContent);
}
}  // namespace
