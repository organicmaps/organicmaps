#include "testing/testing.hpp"

#include "generator/generator_tests_support/routing_helpers.hpp"
#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "generator/restriction_collector.hpp"
#include "generator/restriction_generator.hpp"

#include "routing/restriction_loader.hpp"

#include "indexer/classificator_loader.hpp"

#include "platform/country_file.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

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

void LoadRestrictions(string const & mwmFilePath, vector<Restriction> & restrictions)
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

    RestrictionVec restrictionsNo;
    RestrictionVec restrictionsOnly;
    RestrictionSerializer::Deserialize(header, restrictionsNo, restrictionsOnly, src);

    for (auto const & r : restrictionsNo)
      restrictions.emplace_back(Restriction::Type::No, vector<uint32_t>(r.begin(), r.end()));

    for (auto const & r : restrictionsOnly)
      restrictions.emplace_back(Restriction::Type::Only, vector<uint32_t>(r.begin(), r.end()));
  }
  catch (Reader::OpenException const & e)
  {
    TEST(false, ("Error while reading", RESTRICTIONS_FILE_TAG, "section.", e.Msg()));
  }
}

/// \brief Generates a restriction section, adds it to an empty mwm,
/// loads the restriction section and test loaded restrictions.
/// \param |restrictionPath| comma separated text with restrictions in osm id terms.
/// \param |osmIdsToFeatureIdContent| comma separated text with mapping from osm ids to feature ids.
void TestRestrictionBuilding(string const & restrictionPath,  
                             string const & osmIdsToFeatureIdContent,
                             unique_ptr<IndexGraph> graph,
                             vector<Restriction> & expected)
{
  Platform & platform = GetPlatform();
  string const writableDir = platform.WritableDir();

  string const targetDir = base::JoinPath(writableDir, kTestDir);
  // Building empty mwm.
  LocalCountryFile country(targetDir, CountryFile(kTestMwm), 0 /* version */);
  ScopedDir const scopedDir(kTestDir);
  string const mwmRelativePath = base::JoinPath(kTestDir, kTestMwm + DATA_FILE_EXTENSION);
  ScopedFile const scopedMwm(mwmRelativePath, ScopedFile::Mode::Create);
  BuildEmptyMwm(country);

  // Creating a file with restrictions.
  string const restrictionRelativePath = base::JoinPath(kTestDir, kRestrictionFileName);
  ScopedFile const restrictionScopedFile(restrictionRelativePath, restrictionPath);

  // Creating osm ids to feature ids mapping.
  string const mappingRelativePath = base::JoinPath(kTestDir, kOsmIdsToFeatureIdsName);
  ScopedFile const mappingFile(mappingRelativePath, ScopedFile::Mode::Create);
  string const osmIdsToFeatureIdFullPath = mappingFile.GetFullPath();
  ReEncodeOsmIdsToFeatureIdsMapping(osmIdsToFeatureIdContent, osmIdsToFeatureIdFullPath);

  string const restrictionFullPath = base::JoinPath(writableDir, restrictionRelativePath);
  string const & mwmFullPath = scopedMwm.GetFullPath();

  // Prepare data to collector.
  auto restrictionCollector =
      make_unique<RestrictionCollector>(osmIdsToFeatureIdFullPath, move(graph));

  TEST(restrictionCollector->Process(restrictionFullPath), ("Bad restrictions were given."));

  // Adding restriction section to mwm.
  SerializeRestrictions(*restrictionCollector, mwmFullPath);

  // Reading from mwm section and testing restrictions.
  vector<Restriction> restrictionsFromMwm;
  LoadRestrictions(mwmFullPath, restrictionsFromMwm);

  sort(restrictionsFromMwm.begin(), restrictionsFromMwm.end());
  sort(expected.begin(), expected.end());

  TEST_EQUAL(restrictionsFromMwm, expected, ());
}

// 2                 *
//                ↗     ↘
//              F5        F4
//            ↗              ↘                             Finish
// 1         *                 *<- F3 ->*-> F8 -> *-> F10 -> *
//            ↖                         ↑       ↗
//              F6                      F2   F9
//         Start   ↖                    ↑  ↗
// 0         *-> F7 ->*-> F0 ->*-> F1 ->*
//          -1        0        1        2         3          4
//
pair<unique_ptr<IndexGraph>, string> BuildTwoCubeGraph()
{
  classificator::Load();
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();
  loader->AddRoad(0 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {1.0, 0.0}}));
  loader->AddRoad(1 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 0.0}, {2.0, 0.0}}));
  loader->AddRoad(2 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {2.0, 1.0}}));
  loader->AddRoad(3 /* feature id */, false /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 1.0}, {2.0, 1.0}}));
  loader->AddRoad(4 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 2.0}, {1.0, 1.0}}));
  loader->AddRoad(5 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{-1.0, 1.0}, {0.0, 2.0}}));
  loader->AddRoad(6 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {-1.0, 1.0}}));
  loader->AddRoad(7 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{-1.0, 0.0}, {0.0, 0.0}}));
  loader->AddRoad(8 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 1.0}, {3.0, 1.0}}));
  loader->AddRoad(9 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {3.0, 1.0}}));
  loader->AddRoad(10 /* feature id */, true /* one way */, 1.0 /* speed */,
                  RoadGeometry::Points({{3.0, 1.0}, {4.0, 1.0}}));

  vector<Joint> const joints = {
      // {{/* feature id */, /* point id */}, ... }
      MakeJoint({{7, 0}}),                 /* joint at point (-1, 0) */
      MakeJoint({{0, 0}, {6, 0}, {7, 1}}), /* joint at point (0, 0) */
      MakeJoint({{0, 1}, {1, 0}}),          /* joint at point (1, 0) */
      MakeJoint({{1, 1}, {2, 0}, {9, 0}}),  /* joint at point (2, 0) */
      MakeJoint({{2, 1}, {3, 1}, {8, 0}}),  /* joint at point (2, 1) */
      MakeJoint({{3, 0}, {4, 1}}),          /* joint at point (1, 1) */
      MakeJoint({{5, 1}, {4, 0}}),          /* joint at point (0, 2) */
      MakeJoint({{6, 1}, {5, 0}}),          /* joint at point (-1, 1) */
      MakeJoint({{8, 1}, {9, 1}, {10, 0}}), /* joint at point (3, 1) */
      MakeJoint({{10, 1}})                  /* joint at point (4, 1) */
  };

  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimatorForCar(trafficCache);

  string const osmIdsToFeatureIdsContent = R"(0, 0
                                              1, 1
                                              2, 2
                                              3, 3
                                              4, 4
                                              5, 5
                                              6, 6
                                              7, 7
                                              8, 8
                                              9, 9
                                              10, 10)";

  return {BuildIndexGraph(move(loader), estimator, joints), osmIdsToFeatureIdsContent};
}

UNIT_TEST(RestrictionGenerationTest_1)
{
  string osmIdsToFeatureIdsContent;
  unique_ptr<IndexGraph> indexGraph;
  tie(indexGraph, osmIdsToFeatureIdsContent) = BuildTwoCubeGraph();

  string const restrictionPath =
      /* Type  ViaType  ViaNodeCoords: x    y   from  to */
      R"(Only, node,                   1.0, 0.0,  0,  1)";

  vector<Restriction> expected = {
      {Restriction::Type::Only, {0, 1}}
  };

  TestRestrictionBuilding(restrictionPath, osmIdsToFeatureIdsContent, move(indexGraph),
                          expected);
}

UNIT_TEST(RestrictionGenerationTest_2)
{
  string osmIdsToFeatureIdsContent;
  unique_ptr<IndexGraph> indexGraph;
  tie(indexGraph, osmIdsToFeatureIdsContent) = BuildTwoCubeGraph();

  string const restrictionPath =
      /* Type  ViaType from  ViaWayId to */
      R"(Only, way,      0,     1     2)";

  vector<Restriction> expected = {
      {Restriction::Type::Only, {0, 1, 2}}
  };

  TestRestrictionBuilding(restrictionPath, osmIdsToFeatureIdsContent, move(indexGraph),
                          expected);
}

UNIT_TEST(RestrictionGenerationTest_3)
{
  string osmIdsToFeatureIdsContent;
  unique_ptr<IndexGraph> indexGraph;
  tie(indexGraph, osmIdsToFeatureIdsContent) = BuildTwoCubeGraph();

  string const restrictionPath =
      /* Type  ViaType ViaNodeCoords: x    y   from  to */
      R"(Only, node,                  0.0, 0.0,  7,   6
         No,   way,                              2,   8, 10)";

  vector<Restriction> expected = {
      {Restriction::Type::Only, {7, 6}},
      {Restriction::Type::No,   {2, 8, 10}}
  };

  TestRestrictionBuilding(restrictionPath, osmIdsToFeatureIdsContent, move(indexGraph),
                          expected);
}

UNIT_TEST(RestrictionGenerationTest_BadConnection_1)
{
  string osmIdsToFeatureIdsContent;
  unique_ptr<IndexGraph> indexGraph;
  tie(indexGraph, osmIdsToFeatureIdsContent) = BuildTwoCubeGraph();

  // Here features 7 and 6 don't connect at (2.0, 0.0)
  string const restrictionPath =
      /* Type  ViaType ViaNodeCoords: x    y   from  to */
      R"(Only, node,                  2.0, 0.0,  7,   6
         No,   way,                              2,   8, 10)";

  // So we don't expect first restriction here.
  vector<Restriction> expected = {
      {Restriction::Type::No,   {2, 8, 10}}
  };

  TestRestrictionBuilding(restrictionPath, osmIdsToFeatureIdsContent, move(indexGraph),
                          expected);
}

UNIT_TEST(RestrictionGenerationTest_BadConnection_2)
{
  string osmIdsToFeatureIdsContent;
  unique_ptr<IndexGraph> indexGraph;
  tie(indexGraph, osmIdsToFeatureIdsContent) = BuildTwoCubeGraph();

  // Here features 0, 1 and 3 don't have common joints (namely 1 and 3).
  string const restrictionPath =
      /* Type  ViaType ViaNodeCoords: x    y   from  to */
      R"(Only, node,                  0.0, 0.0,  7,   6
         No,   way,                              0,   1, 3)";

  // So we don't expect second restriction here.
  vector<Restriction> expected = {
      {Restriction::Type::Only,   {7, 6}}
  };

  TestRestrictionBuilding(restrictionPath, osmIdsToFeatureIdsContent, move(indexGraph),
                          expected);
}
}  // namespace
