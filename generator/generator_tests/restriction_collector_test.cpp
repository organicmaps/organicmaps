#include "testing/testing.hpp"

#include "generator/generator_tests/common.hpp"
#include "generator/generator_tests_support/routing_helpers.hpp"
#include "generator/restriction_collector.hpp"

#include "routing/restrictions_serialization.hpp"

#include "traffic/traffic_cache.hpp"

#include "indexer/classificator_loader.hpp"

#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include "base/file_name_utils.hpp"
#include "base/geo_object_id.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_helpers.hpp"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace routing_builder
{
using namespace generator;
using namespace platform;
using namespace platform::tests_support;
using namespace routing;

std::string const kTestDir = "test-restrictions";
std::string const kOsmIdsToFeatureIdsName = "osm_ids_to_feature_ids" OSM2FEATURE_FILE_EXTENSION;

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
std::unique_ptr<IndexGraph> BuildTwoCubeGraph()
{
  classificator::Load();
  std::unique_ptr<TestGeometryLoader> loader = std::make_unique<TestGeometryLoader>();
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

  std::vector<Joint> const joints = {
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

  return BuildIndexGraph(std::move(loader), estimator, joints);
}

std::string const kosmIdsToFeatureIdsContentForTwoCubeGraph =
    R"(0, 0
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

RelationElement MakeRelationElement(std::vector<RelationElement::Member> const & nodes,
                                    std::vector<RelationElement::Member> const & ways,
                                    std::map<std::string, std::string, std::less<>> const & tags)
{
  RelationElement r;
  r.m_nodes = nodes;
  r.m_ways = ways;
  r.m_tags = tags;
  return r;
}

class TestRestrictionCollector
{
public:
  TestRestrictionCollector() : m_scopedDir(kTestDir)
  {
    // Creating osm ids to feature ids mapping.
    std::string const mappingRelativePath = base::JoinPath(kTestDir, kOsmIdsToFeatureIdsName);

    m_scopedFile = std::make_shared<ScopedFile>(mappingRelativePath, ScopedFile::Mode::Create);
    m_osmIdsToFeatureIdFullPath = m_scopedFile->GetFullPath();

    ReEncodeOsmIdsToFeatureIdsMapping(kosmIdsToFeatureIdsContentForTwoCubeGraph, m_osmIdsToFeatureIdFullPath);
  }

  void ValidCase()
  {
    auto graph = BuildTwoCubeGraph();
    RestrictionCollector restrictionCollector(m_osmIdsToFeatureIdFullPath, *graph);

    // Adding restrictions.
    TEST(restrictionCollector.AddRestriction(
             {2.0, 0.0} /* coords of intersection feature with id = 1 and feature with id = 2 */,
             Restriction::Type::No,                     /* restriction type */
             {base::MakeOsmWay(1), base::MakeOsmWay(2)} /* features in format {from, (via*)?, to} */
             ),
         ());

    TEST(restrictionCollector.AddRestriction({2.0, 1.0}, Restriction::Type::Only,
                                             {base::MakeOsmWay(2), base::MakeOsmWay(3)}),
         ());

    TEST(restrictionCollector.AddRestriction(RestrictionCollector::kNoCoords, /* no coords in case of way as via */
                                             Restriction::Type::No,
                                             /*      from                via                    to         */
                                             {base::MakeOsmWay(0), base::MakeOsmWay(1), base::MakeOsmWay(2)}),
         ());

    base::SortUnique(restrictionCollector.m_restrictions);

    std::vector<Restriction> expectedRestrictions = {
        {Restriction::Type::No, {1, 2}},
        {Restriction::Type::Only, {2, 3}},
        {Restriction::Type::No, {0, 1, 2}},
    };

    std::sort(expectedRestrictions.begin(), expectedRestrictions.end());

    TEST_EQUAL(restrictionCollector.m_restrictions, expectedRestrictions, ());
  }

  void InvalidCase_NoSuchFeature()
  {
    auto graph = BuildTwoCubeGraph();
    RestrictionCollector restrictionCollector(m_osmIdsToFeatureIdFullPath, *graph);

    // No such feature - 2809
    TEST(!restrictionCollector.AddRestriction({2.0, 1.0}, Restriction::Type::No,
                                              {base::MakeOsmWay(2809), base::MakeOsmWay(1)}),
         ());

    TEST(!restrictionCollector.HasRestrictions(), ());
  }

  void InvalidCase_FeaturesNotIntersecting()
  {
    auto graph = BuildTwoCubeGraph();
    RestrictionCollector restrictionCollector(m_osmIdsToFeatureIdFullPath, *graph);

    // Fetures with id 1 and 2 do not intersect in {2.0, 1.0}
    TEST(!restrictionCollector.AddRestriction({2.0, 1.0}, Restriction::Type::No,
                                              {base::MakeOsmWay(1), base::MakeOsmWay(2)}),
         ());

    // No such chain of features (1 => 2 => 4),
    // because feature with id 2 and 4 do not have common joint.
    TEST(!restrictionCollector.AddRestriction(RestrictionCollector::kNoCoords, Restriction::Type::No,
                                              {base::MakeOsmWay(1), base::MakeOsmWay(2), base::MakeOsmWay(4)}),
         ());

    TEST(!restrictionCollector.HasRestrictions(), ());
  }

private:
  ScopedDir m_scopedDir;
  std::shared_ptr<ScopedFile> m_scopedFile;
  std::string m_osmIdsToFeatureIdFullPath;
};

UNIT_CLASS_TEST(TestRestrictionCollector, ValidCase)
{
  TestRestrictionCollector::ValidCase();
}

UNIT_CLASS_TEST(TestRestrictionCollector, InvalidCase_NoSuchFeature)
{
  TestRestrictionCollector::InvalidCase_NoSuchFeature();
}

UNIT_CLASS_TEST(TestRestrictionCollector, InvalidCase_FeaturesNotIntersecting)
{
  TestRestrictionCollector::InvalidCase_FeaturesNotIntersecting();
}

UNIT_TEST(RestrictionWriter_Merge)
{
  classificator::Load();
  auto const filename = generator_tests::GetFileName();
  SCOPE_GUARD(_, std::bind(Platform::RemoveFileIfExists, std::cref(filename)));

  auto c1 = std::make_shared<RestrictionWriter>(filename, nullptr /* cache */);
  auto c2 = c1->Clone();
  std::map<std::string, std::string, std::less<>> const tags = {{"type", "restriction"},
                                                                {"restriction", "no_right_turn"}};
  c1->CollectRelation(
      MakeRelationElement({} /* nodes */, {{1, "via"}, {11, "from"}, {21, "to"}} /* ways */, tags /* tags */));
  c2->CollectRelation(
      MakeRelationElement({} /* nodes */, {{2, "via"}, {12, "from"}, {22, "to"}} /* ways */, tags /* tags */));
  c1->CollectRelation(
      MakeRelationElement({} /* nodes */, {{3, "via"}, {13, "from"}, {23, "to"}} /* ways */, tags /* tags */));
  c2->CollectRelation(
      MakeRelationElement({} /* nodes */, {{4, "via"}, {14, "from"}, {24, "to"}} /* ways */, tags /* tags */));
  c1->Finish();
  c2->Finish();
  c1->Merge(*c2);
  c1->Finalize();

  std::ifstream stream;
  stream.exceptions(std::fstream::failbit | std::fstream::badbit);
  stream.open(filename);
  std::stringstream buffer;
  buffer << stream.rdbuf();

  std::string const correctAnswer =
      "No,way,11,1,21\n"
      "No,way,13,3,23\n"
      "No,way,12,2,22\n"
      "No,way,14,4,24\n";
  TEST_EQUAL(buffer.str(), correctAnswer, ());
}
}  // namespace routing_builder
