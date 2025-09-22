#include "testing/testing.hpp"

#include "generator/generator_tests_support/routing_helpers.hpp"
#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"
#include "generator/restriction_collector.hpp"
#include "generator/restriction_generator.hpp"

#include "traffic/traffic_cache.hpp"

#include "indexer/classificator_loader.hpp"

#include "platform/country_file.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include "coding/files_container.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace restriction_test
{
using namespace feature;
using namespace generator;
using namespace platform::tests_support;
using namespace platform;
using namespace routing;
using std::move, std::string, std::unique_ptr, std::vector;

// Directory name for creating test mwm and temporary files.
string const kTestDir = "restriction_generation_test";
// Temporary mwm name for testing.
string const kTestMwm = "test";
string const kRestrictionFileName = "restrictions_in_osm_ids.csv";
string const kOsmIdsToFeatureIdsName = "osm_ids_to_feature_ids" OSM2FEATURE_FILE_EXTENSION;

struct RestrictionUTurnForTests
{
  RestrictionUTurnForTests(Restriction::Type type, uint32_t featureId, bool viaIsFirstPoint)
    : m_type(type)
    , m_featureId(featureId)
    , m_viaIsFirstPoint(viaIsFirstPoint)
  {
    CHECK(m_type == Restriction::Type::NoUTurn || m_type == Restriction::Type::OnlyUTurn, ());
  }

  bool operator==(RestrictionUTurnForTests const & rhs) const
  {
    return m_type == rhs.m_type && m_featureId == rhs.m_featureId && m_viaIsFirstPoint == rhs.m_viaIsFirstPoint;
  }

  bool operator<(RestrictionUTurnForTests const & rhs) const
  {
    if (m_type != rhs.m_type)
      return m_type < rhs.m_type;

    if (m_featureId != rhs.m_featureId)
      return m_featureId < rhs.m_featureId;

    return m_viaIsFirstPoint < rhs.m_viaIsFirstPoint;
  }

  Restriction::Type m_type;
  uint32_t m_featureId;
  bool m_viaIsFirstPoint;
};

string DebugPrint(RestrictionUTurnForTests const & r)
{
  std::ostringstream ss;
  ss << "[" << DebugPrint(r.m_type) << "]: "
     << "feature: " << r.m_featureId << ", "
     << "isFirstPoint: " << r.m_viaIsFirstPoint;

  return ss.str();
}

void BuildEmptyMwm(LocalCountryFile & country)
{
  generator::tests_support::TestMwmBuilder builder(country, feature::DataHeader::MapType::Country);
}

void LoadRestrictions(string const & mwmFilePath, vector<Restriction> & restrictions,
                      vector<RestrictionUTurnForTests> & restrictionsUTurn)
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
    vector<RestrictionUTurn> restrictionsNoUTurn;
    vector<RestrictionUTurn> restrictionsOnlyUTurn;
    RestrictionSerializer::Deserialize(header, restrictionsNo, restrictionsOnly, restrictionsNoUTurn,
                                       restrictionsOnlyUTurn, src);

    for (auto const & r : restrictionsNo)
      restrictions.emplace_back(Restriction::Type::No, vector<uint32_t>(r.begin(), r.end()));

    for (auto const & r : restrictionsOnly)
      restrictions.emplace_back(Restriction::Type::Only, vector<uint32_t>(r.begin(), r.end()));

    for (auto const & r : restrictionsNoUTurn)
      restrictionsUTurn.emplace_back(Restriction::Type::NoUTurn, r.m_featureId, r.m_viaIsFirstPoint);

    for (auto const & r : restrictionsOnlyUTurn)
      restrictionsUTurn.emplace_back(Restriction::Type::OnlyUTurn, r.m_featureId, r.m_viaIsFirstPoint);
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
void TestRestrictionBuilding(string const & restrictionContent, string const & osmIdsToFeatureIdContent,
                             unique_ptr<IndexGraph> graph, vector<Restriction> & expectedNotUTurn,
                             vector<RestrictionUTurnForTests> & expectedUTurn)
{
  string const targetDir = base::JoinPath(GetPlatform().WritableDir(), kTestDir);
  ScopedDirCleanup scopedDir(targetDir);

  // Building empty mwm.
  LocalCountryFile country(targetDir, CountryFile(kTestMwm), 0 /* version */);
  BuildEmptyMwm(country);

  // Creating a file with restrictions.
  ScopedFile const restrictionScopedFile(base::JoinPath(kTestDir, kRestrictionFileName), restrictionContent);

  // Creating osm ids to feature ids mapping.
  string const osmIdsToFeatureIdFullPath = base::JoinPath(targetDir, kOsmIdsToFeatureIdsName);
  ReEncodeOsmIdsToFeatureIdsMapping(osmIdsToFeatureIdContent, osmIdsToFeatureIdFullPath);

  string const restrictionFullPath = base::JoinPath(targetDir, kRestrictionFileName);
  string const mwmFullPath = base::JoinPath(targetDir, kTestMwm + DATA_FILE_EXTENSION);

  // Prepare data to collector.
  auto restrictionCollector =
      std::make_unique<routing_builder::RestrictionCollector>(osmIdsToFeatureIdFullPath, *graph);

  TEST(restrictionCollector->Process(restrictionFullPath), ("Bad restrictions were given."));

  // Adding restriction section to mwm.
  SerializeRestrictions(*restrictionCollector, mwmFullPath);

  // Reading from mwm section and testing restrictions.
  vector<Restriction> restrictionsFromMwm;
  vector<RestrictionUTurnForTests> restrictionsUTurnFromMwm;
  LoadRestrictions(mwmFullPath, restrictionsFromMwm, restrictionsUTurnFromMwm);

  sort(restrictionsFromMwm.begin(), restrictionsFromMwm.end());
  sort(restrictionsUTurnFromMwm.begin(), restrictionsUTurnFromMwm.end());

  sort(expectedNotUTurn.begin(), expectedNotUTurn.end());
  sort(expectedUTurn.begin(), expectedUTurn.end());

  TEST_EQUAL(restrictionsFromMwm, expectedNotUTurn, ());
  TEST_EQUAL(restrictionsUTurnFromMwm, expectedUTurn, ());
}

// 2                 *
//                ↗     ↘
//              F5        F4
//            ↗              ↘                             Finish
// 1         *                 *<- F3 ->*-> F8 -> *-> F10 -> *
//            ↖                         ↑       ↗
//              F6                      F2   F9
//         Start   ↘                    ↑  ↗
// 0         *-> F7 ->*-> F0 ->*-> F1 ->*
//          -1        0        1        2         3          4
//
std::pair<unique_ptr<IndexGraph>, string> BuildTwoCubeGraph()
{
  classificator::Load();
  auto loader = std::make_unique<TestGeometryLoader>();
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
  loader->AddRoad(6 /* feature id */, false /* one way */, 1.0 /* speed */,
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
      MakeJoint({{7, 0}}),                  /* joint at point (-1, 0) */
      MakeJoint({{0, 0}, {6, 0}, {7, 1}}),  /* joint at point (0, 0) */
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
  std::shared_ptr<EdgeEstimator> estimator = CreateEstimatorForCar(trafficCache);

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

  return {BuildIndexGraph(std::move(loader), estimator, joints), osmIdsToFeatureIdsContent};
}

UNIT_TEST(RestrictionGenerationTest_1)
{
  string osmIdsToFeatureIdsContent;
  unique_ptr<IndexGraph> indexGraph;
  tie(indexGraph, osmIdsToFeatureIdsContent) = BuildTwoCubeGraph();

  string const restrictionPath =
      /* Type  ViaType  ViaNodeCoords: x    y   from  to */
      R"(Only, node,                   1.0, 0.0,  0,  1)";

  vector<Restriction> expectedNotUTurn = {{Restriction::Type::Only, {0, 1}}};
  vector<RestrictionUTurnForTests> expectedUTurn;

  TestRestrictionBuilding(restrictionPath, osmIdsToFeatureIdsContent, std::move(indexGraph), expectedNotUTurn,
                          expectedUTurn);
}

UNIT_TEST(RestrictionGenerationTest_2)
{
  string osmIdsToFeatureIdsContent;
  unique_ptr<IndexGraph> indexGraph;
  tie(indexGraph, osmIdsToFeatureIdsContent) = BuildTwoCubeGraph();

  string const restrictionPath =
      /* Type  ViaType from  ViaWayId to */
      R"(Only, way,      0,     1     2)";

  vector<Restriction> expectedNotUTurn = {{Restriction::Type::Only, {0, 1, 2}}};
  vector<RestrictionUTurnForTests> expectedUTurn;

  TestRestrictionBuilding(restrictionPath, osmIdsToFeatureIdsContent, std::move(indexGraph), expectedNotUTurn,
                          expectedUTurn);
}

UNIT_TEST(RestrictionGenerationTest_3)
{
  string osmIdsToFeatureIdsContent;
  unique_ptr<IndexGraph> indexGraph;
  tie(indexGraph, osmIdsToFeatureIdsContent) = BuildTwoCubeGraph();

  string const restrictionPath =
      /* Type  ViaType ViaNodeCoords: x    y   from  via to */
      R"(Only, node,                  0.0, 0.0,  7,      6
         No,   way,                              2,   8, 10)";

  vector<Restriction> expectedNotUTurn = {{Restriction::Type::Only, {7, 6}}, {Restriction::Type::No, {2, 8, 10}}};
  vector<RestrictionUTurnForTests> expectedUTurn;

  TestRestrictionBuilding(restrictionPath, osmIdsToFeatureIdsContent, std::move(indexGraph), expectedNotUTurn,
                          expectedUTurn);
}

UNIT_TEST(RestrictionGenerationTest_BadConnection_1)
{
  string osmIdsToFeatureIdsContent;
  unique_ptr<IndexGraph> indexGraph;
  tie(indexGraph, osmIdsToFeatureIdsContent) = BuildTwoCubeGraph();

  // Here features 7 and 6 don't connect at (2.0, 0.0)
  string const restrictionPath =
      /* Type  ViaType ViaNodeCoords: x    y   from  via to */
      R"(Only, node,                  2.0, 0.0,  7,      6
         No,   way,                              2,   8, 10)";

  // So we don't expect first restriction here.
  vector<Restriction> expectedNotUTurn = {{Restriction::Type::No, {2, 8, 10}}};
  vector<RestrictionUTurnForTests> expectedUTurn;

  TestRestrictionBuilding(restrictionPath, osmIdsToFeatureIdsContent, std::move(indexGraph), expectedNotUTurn,
                          expectedUTurn);
}

UNIT_TEST(RestrictionGenerationTest_BadConnection_2)
{
  string osmIdsToFeatureIdsContent;
  unique_ptr<IndexGraph> indexGraph;
  tie(indexGraph, osmIdsToFeatureIdsContent) = BuildTwoCubeGraph();

  // Here features 0, 1 and 3 don't have common joints (namely 1 and 3).
  string const restrictionPath =
      /* Type  ViaType ViaNodeCoords: x    y   from  via to */
      R"(Only, node,                  0.0, 0.0,  7,      6
         No,   way,                              0,   1, 3)";

  // So we don't expect second restriction here.
  vector<Restriction> expectedNotUTurn = {{Restriction::Type::Only, {7, 6}}};
  vector<RestrictionUTurnForTests> expectedUTurn;

  TestRestrictionBuilding(restrictionPath, osmIdsToFeatureIdsContent, std::move(indexGraph), expectedNotUTurn,
                          expectedUTurn);
}

UNIT_TEST(RestrictionGenerationTest_WithUTurn_1)
{
  string osmIdsToFeatureIdsContent;
  unique_ptr<IndexGraph> indexGraph;
  tie(indexGraph, osmIdsToFeatureIdsContent) = BuildTwoCubeGraph();

  string const restrictionPath =
      /* Type     ViaType ViaNodeCoords: x    y   from  to */
      R"(NoUTurn,   node,                2.0, 1.0,  3,   3
       OnlyUTurn, node,                0.0, 0.0,  6,   6)";

  // So we don't expect second restriction here.
  vector<Restriction> expectedNotUTurn;
  vector<RestrictionUTurnForTests> expectedUTurn = {
      {Restriction::Type::NoUTurn, 3 /* featureId */, false /* viaIsFirstPoint */},
      {Restriction::Type::OnlyUTurn, 6 /* featureId */, true /* viaIsFirstPoint */}};

  TestRestrictionBuilding(restrictionPath, osmIdsToFeatureIdsContent, std::move(indexGraph), expectedNotUTurn,
                          expectedUTurn);
}

UNIT_TEST(RestrictionGenerationTest_WithUTurn_2)
{
  string osmIdsToFeatureIdsContent;
  unique_ptr<IndexGraph> indexGraph;
  tie(indexGraph, osmIdsToFeatureIdsContent) = BuildTwoCubeGraph();

  // First two are not UTurn restrictions, but still correct restriction.
  // We should just convert them.
  string const restrictionPath =
      /* Type     ViaType ViaNodeCoords: x    y   from via to */
      R"(NoUTurn,   node,                2.0, 1.0,  2,     8
         NoUTurn,    way,                           2, 8,  10
         OnlyUTurn,  way,                           6, 5,  4
         OnlyUTurn, node,               -1.0, 1.0,  6,     6)";

  // So we don't expect second restriction here.
  vector<Restriction> expectedNotUTurn = {
      {Restriction::Type::No, {2, 8}}, {Restriction::Type::No, {2, 8, 10}}, {Restriction::Type::Only, {6, 5, 4}}};

  vector<RestrictionUTurnForTests> expectedUTurn = {
      {Restriction::Type::OnlyUTurn, 6 /* featureId */, false /* viaIsFirstPoint */}};

  TestRestrictionBuilding(restrictionPath, osmIdsToFeatureIdsContent, std::move(indexGraph), expectedNotUTurn,
                          expectedUTurn);
}

UNIT_TEST(RestrictionGenerationTest_WithUTurn_BadConnection_1)
{
  string osmIdsToFeatureIdsContent;
  unique_ptr<IndexGraph> indexGraph;
  tie(indexGraph, osmIdsToFeatureIdsContent) = BuildTwoCubeGraph();

  string const restrictionPath =
      /* Type     ViaType ViaNodeCoords: x     y   from  via  to */
      R"(NoUTurn,   node,                20.0, 11.0,  2,      8
         OnlyUTurn,  way,                             6,   2, 4
         OnlyUTurn, node,               -10.0, 10.0,  6,      6
         NoUTurn,   node,               -10.0, 10.0,  6,      6)";

  // So we don't expect second restriction here.
  vector<Restriction> expectedNotUTurn;
  vector<RestrictionUTurnForTests> expectedUTurn;

  TestRestrictionBuilding(restrictionPath, osmIdsToFeatureIdsContent, std::move(indexGraph), expectedNotUTurn,
                          expectedUTurn);
}
}  // namespace restriction_test
