#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_feature.cpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"
#include "generator/maxspeed_builder.hpp"
#include "generator/routing_helpers.hpp"

#include "routing/maxspeed_serialization.hpp"
#include "routing/maxspeeds.hpp"

#include "routing_common/maxspeed_conversion.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/mwm_set.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "geometry/point2d.hpp"

#include "platform/local_country_file.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include "base/geo_object_id.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace
{
using namespace measurement_utils;
using namespace platform::tests_support;
using namespace platform;
using namespace routing;
using namespace std;

using FeatureVector = vector<vector<m2::PointD>>;

// Directory name for creating test mwm and temporary files.
string const kTestDir = "maxspeed_generation_test";
// Temporary mwm name for testing.
string const kTest = "test";
// File name for keeping maxspeeds.
string const kCsv = "maxspeed.csv";

void BuldGeometry(FeatureVector const & roads, LocalCountryFile & country)
{
  generator::tests_support::TestMwmBuilder builder(country, feature::DataHeader::country);
  for (auto const & road : roads)
    builder.Add(generator::tests_support::TestStreet(road, string(), string()));
}

void CreateTextFileByContent(string const & fileName, string const & fileContent)
{
  ofstream ofs(fileName);
  TEST(ofs, ());
  ofs << fileContent;
}

void TestMaxspeedSection(FeatureVector const & roads, string const & maxspeedCsvContent,
                         map<uint32_t, base::GeoObjectId> const & featureIdToOsmId)
{
  classificator::Load();
  string const testDirFullPath = base::JoinPath(GetPlatform().WritableDir(), kTestDir);
  ScopedDir testScopedDir(kTestDir);

  // Writing |maxspeedCsvContent| to a file in |kTestDir|.
  ScopedFile testScopedMaxspeedCsv(base::JoinPath(kTestDir, kCsv), ScopedFile::Mode::Create);
  string const csvFullPath = base::JoinPath(testDirFullPath, kCsv);
  CreateTextFileByContent(csvFullPath, maxspeedCsvContent);

  // Writing |roads| to test mwm.
  LocalCountryFile country(testDirFullPath, CountryFile(kTest), 1);
  string const testMwm = kTest + DATA_FILE_EXTENSION;
  ScopedFile testScopedMwm(base::JoinPath(kTestDir, testMwm), ScopedFile::Mode::Create);
  BuldGeometry(roads, country);

  // Creating maxspeed section in test.mwm.
  string const testMwmFullPath = base::JoinPath(testDirFullPath, testMwm);
  BuildMaxspeed(testMwmFullPath, featureIdToOsmId, csvFullPath);

  // Loading maxspeed section.
  FrozenDataSource dataSource;
  auto const regResult = dataSource.RegisterMap(country);
  TEST_EQUAL(regResult.second, MwmSet::RegResult::Success, ());
  auto const & mwmId = regResult.first;
  auto const handle = dataSource.GetMwmHandleById(mwmId);
  TEST(handle.IsAlive(), ());

  auto const maxspeeds = LoadMaxspeeds(dataSource, handle);
  TEST(maxspeeds, ());

  // Testing maxspeed section content.
  auto const featureNumber = roads.size();
  TEST(!maxspeeds->HasMaxspeed(static_cast<uint32_t>(featureNumber + 1)), ());

  OsmIdToMaxspeed osmIdToMaxspeed;
  TEST(ParseMaxspeeds(base::JoinPath(testDirFullPath, kCsv), osmIdToMaxspeed), ());
  auto processor = [&](FeatureType & f, uint32_t const & id) {
    TEST(IsRoad(feature::TypesHolder(f)), ());

    // Looking for maxspeed from csv.
    auto const itOsmId = featureIdToOsmId.find(id);
    TEST(itOsmId != featureIdToOsmId.cend(), ());

    auto const itMaxspeedCsv = osmIdToMaxspeed.find(itOsmId->second);
    if (itMaxspeedCsv == osmIdToMaxspeed.cend())
      return; // No maxspeed for feature |id|.

    TEST(maxspeeds->HasMaxspeed(id), ());
    Maxspeed const maxspeedCsv = itMaxspeedCsv->second;
    Maxspeed const maxspeedMwm = maxspeeds->GetMaxspeed(id);

    // Comparing maxspeed from csv and maxspeed from mwm section.
    TEST_EQUAL(maxspeedCsv, maxspeedMwm, ());
  };
  feature::ForEachFromDat(testMwmFullPath, processor);
}

// Note. ParseMaxspeeds() is not tested in TestMaxspeedSection() because it's used twice there.
// So it's important to test the function separately.
bool ParseCsv(string const & maxspeedCsvContent, OsmIdToMaxspeed & mapping)
{
  string const testDirFullPath = base::JoinPath(GetPlatform().WritableDir(), kTestDir);
  ScopedDir testScopedDir(kTestDir);

  ScopedFile testScopedMaxspeedCsv(base::JoinPath(kTestDir, kCsv), ScopedFile::Mode::Create);
  CreateTextFileByContent(base::JoinPath(testDirFullPath, kCsv), maxspeedCsvContent);

  return ParseMaxspeeds(base::JoinPath(testDirFullPath, kCsv), mapping);
}

UNIT_TEST(ParseMaxspeeds_Smoke)
{
  string const maxspeedCsvContent;
  OsmIdToMaxspeed osmIdToMaxspeed;
  TEST(ParseCsv(maxspeedCsvContent, osmIdToMaxspeed), ());
  TEST(osmIdToMaxspeed.empty(), ());
}

UNIT_TEST(ParseMaxspeeds1)
{
  string const maxspeedCsvContent = "10,Metric,60\n11,Metric,90\n";
  OsmIdToMaxspeed const expectedMapping = {
      {base::MakeOsmWay(10), {Units::Metric, 60, kInvalidSpeed}},
      {base::MakeOsmWay(11), {Units::Metric, 90, kInvalidSpeed}}};
  OsmIdToMaxspeed osmIdToMaxspeed;
  TEST(ParseCsv(maxspeedCsvContent, osmIdToMaxspeed), ());
  TEST_EQUAL(osmIdToMaxspeed, expectedMapping, ());
}

UNIT_TEST(ParseMaxspeeds2)
{
  string const maxspeedCsvContent = "10,Metric,60,80\n11,Metric,120\n";
  OsmIdToMaxspeed const expectedMapping = {
      {base::MakeOsmWay(10), {Units::Metric, 60, 80}},
      {base::MakeOsmWay(11), {Units::Metric, 120, kInvalidSpeed}}};
  OsmIdToMaxspeed osmIdToMaxspeed;
  TEST(ParseCsv(maxspeedCsvContent, osmIdToMaxspeed), ());
  TEST_EQUAL(osmIdToMaxspeed, expectedMapping, ());
}

UNIT_TEST(ParseMaxspeeds3)
{
  string const maxspeedCsvContent = "184467440737095516,Imperial,60,80\n184467440737095517,Metric,120\n";
  OsmIdToMaxspeed const expectedMapping = {
      {base::MakeOsmWay(184467440737095516), {Units::Imperial, 60, 80}},
      {base::MakeOsmWay(184467440737095517), {Units::Metric, 120, kInvalidSpeed}}};
  OsmIdToMaxspeed osmIdToMaxspeed;
  TEST(ParseCsv(maxspeedCsvContent, osmIdToMaxspeed), ());
  TEST_EQUAL(osmIdToMaxspeed, expectedMapping, ());
}

UNIT_TEST(ParseMaxspeeds4)
{
  // Note. kNoneMaxSpeed == 65534 and kWalkMaxSpeed == 65533.
  string const maxspeedCsvContent = "1,Metric,200,65534\n2,Metric,65533\n";
  OsmIdToMaxspeed const expectedMapping = {
      {base::MakeOsmWay(1), {Units::Metric, 200, kNoneMaxSpeed}},
      {base::MakeOsmWay(2), {Units::Metric, kWalkMaxSpeed, kInvalidSpeed}}};
  OsmIdToMaxspeed osmIdToMaxspeed;
  TEST(ParseCsv(maxspeedCsvContent, osmIdToMaxspeed), ());
  TEST_EQUAL(osmIdToMaxspeed, expectedMapping, ());
}

UNIT_TEST(ParseMaxspeeds5)
{
  string const maxspeedCsvContent = "\n2,Metric,10\n";
  OsmIdToMaxspeed osmIdToMaxspeed;
  TEST(!ParseCsv(maxspeedCsvContent, osmIdToMaxspeed), ());
}

UNIT_TEST(ParseMaxspeeds6)
{
  string const maxspeedCsvContent = "2U,Metric,10\n";
  OsmIdToMaxspeed osmIdToMaxspeed;
  TEST(!ParseCsv(maxspeedCsvContent, osmIdToMaxspeed), ());
}

UNIT_TEST(ParseMaxspeeds7)
{
  string const maxspeedCsvContent = "2,Metric\n";
  OsmIdToMaxspeed osmIdToMaxspeed;
  TEST(!ParseCsv(maxspeedCsvContent, osmIdToMaxspeed), ());
}

UNIT_TEST(ParseMaxspeeds8)
{
  string const maxspeedCsvContent = "2,Metric,10,11m\n";
  OsmIdToMaxspeed osmIdToMaxspeed;
  TEST(!ParseCsv(maxspeedCsvContent, osmIdToMaxspeed), ());
}

UNIT_TEST(ParseMaxspeeds_Big)
{
  // Note. kNoneMaxSpeed == 65534.
  string const maxspeedCsvContent = "100,Metric,200,65534\n101,Metric,60,90\n102,Metric,60\n103,Metric,90\n";
  OsmIdToMaxspeed const expectedMapping = {
      {base::MakeOsmWay(100), {Units::Metric, 200, kNoneMaxSpeed}},
      {base::MakeOsmWay(101), {Units::Metric, 60, 90}},
      {base::MakeOsmWay(102), {Units::Metric, 60, kInvalidSpeed}},
      {base::MakeOsmWay(103), {Units::Metric, 90, kInvalidSpeed}}};
  OsmIdToMaxspeed osmIdToMaxspeed;
  TEST(ParseCsv(maxspeedCsvContent, osmIdToMaxspeed), ());
  TEST_EQUAL(osmIdToMaxspeed, expectedMapping, ());
}

UNIT_TEST(MaxspeedSection_Smoke)
{
  FeatureVector const roads;
  string const maxspeedCsvContent;
  map<uint32_t, base::GeoObjectId> const featureIdToOsmId;
  TestMaxspeedSection(roads, maxspeedCsvContent, featureIdToOsmId);
}

UNIT_TEST(MaxspeedSection1)
{
  FeatureVector const roads = {{{0.0, 0.0}, {0.0, 1.0}, {0.0, 2.0}} /* Points of feature 0 */,
                               {{1.0, 0.0}, {1.0, 1.0}, {1.0, 2.0}} /* Points of feature 1 */};
  string const maxspeedCsvContent = "25258932,Metric,60\n25258943,Metric,90\n";
  map<uint32_t, base::GeoObjectId> const featureIdToOsmId = {
      {0 /* feature id */, base::MakeOsmWay(25258932)},
      {1 /* feature id */, base::MakeOsmWay(25258943)}};
  TestMaxspeedSection(roads, maxspeedCsvContent, featureIdToOsmId);
}

UNIT_TEST(MaxspeedSection2)
{
  FeatureVector const roads = {{{0.0, 0.0}, {0.0, 1.0}} /* Points of feature 0 */,
                               {{1.0, 0.0}, {1.0, 2.0}} /* Points of feature 1 */,
                               {{1.0, 2.0}, {1.0, 3.0}} /* Points of feature 2 */};
  string const maxspeedCsvContent = "25258932,Metric,60,40\n32424,Metric,120\n";
  map<uint32_t, base::GeoObjectId> const featureIdToOsmId = {
      {0 /* feature id */, base::MakeOsmWay(25258932)},
      {1 /* feature id */, base::MakeOsmWay(25258943)},
      {2 /* feature id */, base::MakeOsmWay(32424)}};
  TestMaxspeedSection(roads, maxspeedCsvContent, featureIdToOsmId);
}

UNIT_TEST(MaxspeedSection3)
{
  FeatureVector const roads = {{{0.0, 0.0}, {0.0, 1.0}} /* Points of feature 0 */,
                               {{1.0, 0.0}, {1.0, 2.0}} /* Points of feature 1 */,
                               {{1.0, 2.0}, {1.0, 3.0}} /* Points of feature 2 */};
  // Note. kNoneMaxSpeed == 65535 and kWalkMaxSpeed == 65534.
  string const maxspeedCsvContent =
      "25252,Metric,120,65534\n258943,Metric,65533\n32424,Metric,10,65533\n";
  map<uint32_t, base::GeoObjectId> const featureIdToOsmId = {
      {0 /* feature id */, base::MakeOsmWay(25252)},
      {1 /* feature id */, base::MakeOsmWay(258943)},
      {2 /* feature id */, base::MakeOsmWay(32424)}};
  TestMaxspeedSection(roads, maxspeedCsvContent, featureIdToOsmId);
}

UNIT_TEST(MaxspeedSection4)
{
  FeatureVector const roads = {{{0.0, 0.0}, {0.0, 1.0}} /* Points of feature 0 */,
                               {{1.0, 0.0}, {0.0, 0.0}} /* Points of feature 1 */};
  string const maxspeedCsvContent = "50000000000,Imperial,30\n50000000001,Imperial,50\n";
  map<uint32_t, base::GeoObjectId> const featureIdToOsmId = {
      {0 /* feature id */, base::MakeOsmWay(50000000000)},
      {1 /* feature id */, base::MakeOsmWay(50000000001)}};
  TestMaxspeedSection(roads, maxspeedCsvContent, featureIdToOsmId);
}

UNIT_TEST(MaxspeedSection_Big)
{
  FeatureVector const roads = {{{0.0, 0.0}, {0.0, 1.0}} /* Points of feature 0 */,
                               {{1.0, 0.0}, {1.0, 2.0}} /* Points of feature 1 */,
                               {{1.0, 2.0}, {1.0, 3.0}} /* Points of feature 2 */,
                               {{1.0, 2.0}, {1.0, 4.0}} /* Points of feature 3 */,
                               {{1.0, 2.0}, {2.0, 3.0}} /* Points of feature 4 */,
                               {{1.0, 2.0}, {2.0, 7.0}} /* Points of feature 5 */,
                               {{1.0, 2.0}, {7.0, 4.0}} /* Points of feature 6 */};
  // Note. kNoneMaxSpeed == 65534.
  string const maxspeedCsvContent =
      "100,Imperial,100,65534\n200,Imperial,50\n300,Imperial,30\n400,Imperial,10,20\n600,"
      "Imperial,50,20\n700,Imperial,10\n";
  map<uint32_t, base::GeoObjectId> const featureIdToOsmId = {
      {0 /* feature id */, base::MakeOsmWay(100)}, {1 /* feature id */, base::MakeOsmWay(200)},
      {2 /* feature id */, base::MakeOsmWay(300)}, {3 /* feature id */, base::MakeOsmWay(400)},
      {4 /* feature id */, base::MakeOsmWay(500)}, {5 /* feature id */, base::MakeOsmWay(600)},
      {6 /* feature id */, base::MakeOsmWay(700)}};
  TestMaxspeedSection(roads, maxspeedCsvContent, featureIdToOsmId);
}
}  // namespace
