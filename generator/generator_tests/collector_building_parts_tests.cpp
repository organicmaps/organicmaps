#include "testing/testing.hpp"

#include "generator/collector_building_parts.hpp"
#include "generator/feature_builder.hpp"
#include "generator/gen_mwm_info.hpp"
#include "generator/generator_tests_support/test_with_classificator.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"

#include "indexer/classificator.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include <memory>
#include <unordered_map>

namespace collector_building_parts_tests
{
using namespace generator::tests_support;

class TestOSMElementCacheReader : public generator::cache::OSMElementCacheReaderInterface
{
public:
  TestOSMElementCacheReader(std::unordered_map<generator::cache::Key, RelationElement> & m) : m_mapping(m) {}

  // OSMElementCacheReaderInterface overrides:
  bool Read(generator::cache::Key /* id */, WayElement & /* value */) override { UNREACHABLE(); }

  bool Read(generator::cache::Key id, RelationElement & value) override
  {
    auto const it = m_mapping.find(id);
    if (it == std::cend(m_mapping))
      return false;

    value = it->second;
    return true;
  }

private:
  std::unordered_map<generator::cache::Key, RelationElement> & m_mapping;
};

class IntermediateDataReaderTest : public generator::cache::IntermediateDataReaderInterface
{
public:
  using IdToIds = std::unordered_map<generator::cache::Key, std::vector<generator::cache::Key>>;

  static generator::cache::Key const kTopRelationId1;
  static generator::cache::Key const kOutlineId1;
  static generator::cache::Key const kTopRelationId2;
  static generator::cache::Key const kOutlineId2;

  IntermediateDataReaderTest()
  {
    {
      RelationElement topRelationElement;
      topRelationElement.m_tags = {{"type", "building"}};
      topRelationElement.m_relations = {
          {kOutlineId1, "outline"},
          {3271043, "part"},
          {3271041, "part"},
      };
      for (auto const & p : topRelationElement.m_relations)
        m_relationToRelations[p.first].emplace_back(kTopRelationId1);

      topRelationElement.m_ways = {
          {292789674, "part"}, {242078027, "part"}, {242078028, "part"},
          {242077956, "part"}, {242077935, "part"}, {242077967, "part"},
      };
      for (auto const & p : topRelationElement.m_ways)
        m_wayToRelations[p.first].emplace_back(kTopRelationId1);

      m_IdToRelation[kTopRelationId1] = std::move(topRelationElement);
    }
    {
      RelationElement topRelationElement;
      topRelationElement.m_tags = {{"type", "building"}};
      topRelationElement.m_relations = {
          {kOutlineId2, "outline"},
      };
      for (auto const & p : topRelationElement.m_relations)
        m_relationToRelations[p.first].emplace_back(kTopRelationId2);

      topRelationElement.m_ways = {
          {392789674, "part"}, {342078027, "part"}, {342078028, "part"},
          {342077956, "part"}, {342077935, "part"}, {342077967, "part"},
      };
      for (auto const & p : topRelationElement.m_ways)
        m_wayToRelations[p.first].emplace_back(kTopRelationId2);

      m_IdToRelation[kTopRelationId2] = std::move(topRelationElement);
    }
  }

  // IntermediateDataReaderBase overrides:
  bool GetNode(generator::cache::Key, double &, double &) const override { UNREACHABLE(); }

  bool GetWay(generator::cache::Key /* id */, WayElement & /* e */) override { UNREACHABLE(); }

  bool GetRelation(generator::cache::Key id, RelationElement & e) override
  {
    auto const it = m_IdToRelation.find(id);
    if (it == std::cend(m_IdToRelation))
      return false;

    e = it->second;
    return true;
  }

  void ForEachRelationByWayCached(generator::cache::Key id, ForEachRelationFn & toDo) override
  {
    ForEachRelationById(id, toDo, m_wayToRelations);
  }

  void ForEachRelationByRelationCached(generator::cache::Key id, ForEachRelationFn & toDo) override
  {
    ForEachRelationById(id, toDo, m_relationToRelations);
  }

private:
  void ForEachRelationById(generator::cache::Key id, ForEachRelationFn & toDo, IdToIds const & m)
  {
    auto const it = m.find(id);
    if (it == std::cend(m))
      return;

    TestOSMElementCacheReader reader(m_IdToRelation);
    for (auto id : it->second)
      toDo(id, reader);
  }

  std::unordered_map<generator::cache::Key, RelationElement> m_IdToRelation;
  IdToIds m_wayToRelations;
  IdToIds m_relationToRelations;
};

// static
generator::cache::Key const IntermediateDataReaderTest::kTopRelationId1 = 3271044;
// static
generator::cache::Key const IntermediateDataReaderTest::kOutlineId1 = 1360656;
// static
generator::cache::Key const IntermediateDataReaderTest::kTopRelationId2 = 4271044;
// static
generator::cache::Key const IntermediateDataReaderTest::kOutlineId2 = 2360656;

void TestCollector(std::string const & filename, feature::FeatureBuilder const & fb,
                   IntermediateDataReaderTest & reader, generator::cache::Key topRelationId)
{
  generator::BuildingToBuildingPartsMap m(filename);
  auto const & parts = m.GetBuildingPartsByOutlineId(generator::MakeCompositeId(fb));
  TEST(!parts.empty(), ());

  RelationElement relation;
  TEST(reader.GetRelation(topRelationId, relation), ());
  for (auto const & k : relation.m_ways)
  {
    if (k.second != "part")
      continue;

    auto const id = base::MakeOsmWay(k.first);
    TEST(m.HasBuildingPart(id), ());
    auto const it = std::find(std::cbegin(parts), std::cend(parts), id);
    TEST(it != std::cend(parts), ());
  }
  for (auto const & k : relation.m_relations)
  {
    if (k.second != "part")
      continue;

    auto const id = base::MakeOsmRelation(k.first);
    TEST(m.HasBuildingPart(id), ());
    auto const it = std::find(std::cbegin(parts), std::cend(parts), id);
    TEST(it != std::cend(parts), ());
  }
}

UNIT_CLASS_TEST(TestWithClassificator, CollectorBuildingParts_Case1)
{
  using namespace platform::tests_support;
  ScopedFile file("CollectorBuildingParts", ScopedFile::Mode::DoNotCreate);
  auto intermediateReader = std::make_shared<IntermediateDataReaderTest>();

  feature::FeatureBuilder fb;
  fb.AddOsmId(base::MakeOsmRelation(IntermediateDataReaderTest::kOutlineId1));
  fb.AddType(classif().GetTypeByPath({"building"}));
  fb.SetArea();
  {
    generator::BuildingPartsCollector collector(file.GetFullPath(), intermediateReader);
    collector.CollectFeature(fb, OsmElement());
    collector.Finish();
    collector.Finalize();
  }
  TestCollector(file.GetFullPath(), fb, *intermediateReader, IntermediateDataReaderTest::kTopRelationId1);
}

UNIT_CLASS_TEST(TestWithClassificator, CollectorBuildingParts_Case2)
{
  using namespace platform::tests_support;
  ScopedFile file("CollectorBuildingParts", ScopedFile::Mode::DoNotCreate);

  feature::FeatureBuilder fb1;
  fb1.AddOsmId(base::MakeOsmRelation(IntermediateDataReaderTest::kOutlineId1));
  fb1.AddType(classif().GetTypeByPath({"building"}));
  fb1.SetArea();

  feature::FeatureBuilder fb2;
  fb2.AddOsmId(base::MakeOsmRelation(IntermediateDataReaderTest::kOutlineId2));
  fb2.AddType(classif().GetTypeByPath({"building"}));
  fb2.SetArea();

  auto intermediateReader = std::make_shared<IntermediateDataReaderTest>();
  {
    auto collector1 = std::make_shared<generator::BuildingPartsCollector>(file.GetFullPath(), intermediateReader);
    // We don't clone cache, because it isn't mutable.
    auto collector2 = collector1->Clone(intermediateReader);

    collector1->CollectFeature(fb1, OsmElement());
    collector1->Finish();

    collector2->CollectFeature(fb2, OsmElement());
    collector2->Finish();

    collector1->Merge(*collector2);
    collector1->Finalize();
  }

  TestCollector(file.GetFullPath(), fb1, *intermediateReader, IntermediateDataReaderTest::kTopRelationId1);
  TestCollector(file.GetFullPath(), fb2, *intermediateReader, IntermediateDataReaderTest::kTopRelationId2);
}
}  // namespace collector_building_parts_tests
