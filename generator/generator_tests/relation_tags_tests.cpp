#include "testing/testing.hpp"

#include "generator/generator_tests/common.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/relation_tags.hpp"

#include "indexer/classificator_loader.hpp"

// TODO: Rewrite these tests using RelationTagsEnricher with some test mock of IntermediateDataReaderInterface.
namespace relation_tags_tests
{
using namespace feature;
using namespace generator;
using namespace generator::cache;
using namespace generator_tests;

// In memory relations storage (copy-n-paste from collector_building_parts_tests.cpp).
class TestOSMElementCacheReader : public OSMElementCacheReaderInterface
{
public:
  TestOSMElementCacheReader(std::unordered_map<Key, RelationElement> & m) : m_mapping(m) {}

  // OSMElementCacheReaderInterface overrides:
  bool Read(Key /* id */, WayElement & /* value */) override { UNREACHABLE(); }

  bool Read(Key id, RelationElement & value) override
  {
    auto const it = m_mapping.find(id);
    if (it == std::cend(m_mapping))
      return false;

    value = it->second;
    return true;
  }

private:
  std::unordered_map<Key, RelationElement> & m_mapping;
};

UNIT_TEST(Process_route_with_ref)
{
  /* Prepare relations data:
   * Relation 1:
   *   - type = route
   *   - route = road
   *   - ref = E-99
   *   - members: [
   *       Way 10:
   *         - highway = motorway
   *       Way 11:
   *         - highway = motorway
   *         - ref = F-16
   *     ]
   */

  // Create relation.
  std::vector<RelationElement::Member> testMembers = {{10, ""}, {11, ""}};

  RelationElement e1;
  e1.m_ways = testMembers;
  e1.m_tags.emplace("type", "route");
  e1.m_tags.emplace("route", "road");
  e1.m_tags.emplace("ref", "E-99");

  std::unordered_map<Key, RelationElement> m_IdToRelation = {{1, e1}};
  TestOSMElementCacheReader reader(m_IdToRelation);

  // Create roads.
  auto road10 = MakeOsmElement(10, {{"highway", "motorway"}}, OsmElement::EntityType::Way);
  auto road11 = MakeOsmElement(11, {{"highway", "motorway"}, {"ref", "F-16"}}, OsmElement::EntityType::Way);

  // Process roads tags using relation tags.
  RelationTagsWay rtw;

  rtw.Reset(10, &road10);
  rtw(1, reader);

  rtw.Reset(11, &road11);
  rtw(1, reader);

  // Verify roads tags.
  TEST_EQUAL(road10.GetTag("ref"), "E-99", ());
  TEST_EQUAL(road11.GetTag("ref"), "F-16", ());
}

UNIT_TEST(Process_route_with_ref_network)
{
  /* Prepare relations data:
   * Relation 1:
   *   - type = route
   *   - route = road
   *   - ref = SP60
   *   - network = IT:RA
   *   - members: [
   *       Way 10:
   *         - highway = motorway
   *         - name = Via Corleto
   *       Way 11:
   *         - highway = motorway
   *         - ref = SP62
   *     ]
   */

  // Create relation.
  std::vector<RelationElement::Member> testMembers = {{10, ""}, {11, ""}};

  RelationElement e1;
  e1.m_ways = testMembers;
  e1.m_tags.emplace("type", "route");
  e1.m_tags.emplace("route", "road");
  e1.m_tags.emplace("ref", "SP60");
  e1.m_tags.emplace("network", "IT:RA");

  std::unordered_map<Key, RelationElement> m_IdToRelation = {{1, e1}};
  TestOSMElementCacheReader reader(m_IdToRelation);

  // Create roads.
  auto road10 = MakeOsmElement(10, {{"highway", "motorway"}, {"name", "Via Corleto"}}, OsmElement::EntityType::Way);
  auto road11 = MakeOsmElement(11, {{"highway", "motorway"}, {"ref", "SP62"}}, OsmElement::EntityType::Way);

  // Process roads tags using relation tags.
  RelationTagsWay rtw;

  rtw.Reset(10, &road10);
  rtw(1, reader);

  rtw.Reset(11, &road11);
  rtw(1, reader);

  // Verify roads tags.
  TEST_EQUAL(road10.GetTag("ref"), "IT:RA/SP60", ());
  TEST_EQUAL(road11.GetTag("ref"), "SP62", ());  // TODO: Check refs inheritance (expected "IT:RA/SP60;SP62")
}

UNIT_TEST(Process_associatedStreet)
{
  /* Prepare relations data:
   * Relation 1:
   *   - type = associatedStreet
   *   - name = Main street
   *   - wikipedia = en:Main Street
   *   - place =
   *   - members: [
   *       Way 2:
   *         - building = yes
   *         - addr:housenumber = 121
   *       Way 3:
   *         - building = house
   *         - addr:housenumber = 123
   *         - addr:street = The Main Street
   *     ]
   */

  // Create relation.
  std::vector<RelationElement::Member> testMembers = {{2, "house"}, {3, "house"}, {4, "street"}};

  RelationElement e1;
  e1.m_ways = testMembers;
  e1.m_tags.emplace("type", "associatedStreet");
  e1.m_tags.emplace("name", "Main Street");
  e1.m_tags.emplace("wikipedia", "en:Main Street");

  std::unordered_map<Key, RelationElement> m_IdToRelation = {{1, e1}};
  TestOSMElementCacheReader reader(m_IdToRelation);

  // Create buildings polygons.
  auto buildingWay2 =
      MakeOsmElement(2, {{"building", "yes"}, {"addr:housenumber", "121"}}, OsmElement::EntityType::Way);
  auto buildingWay3 = MakeOsmElement(3,
                                     {{"shop", "convenience"},
                                      {"addr:housenumber", "123"},
                                      {"addr:street", "The Main Street"},
                                      {"wikipedia", "en:Mega Theater"}},
                                     OsmElement::EntityType::Way);
  auto highway4 = MakeOsmElement(4, {{"highway", "residential"}}, OsmElement::EntityType::Way);

  // Process buildings tags using relation tags.
  RelationTagsWay rtw;

  rtw.Reset(2, &buildingWay2);
  rtw(1, reader);

  rtw.Reset(3, &buildingWay3);
  rtw(1, reader);

  rtw.Reset(4, &highway4);
  rtw(1, reader);

  // Aggreagte wiki from associatedStreet only for highway.
  TEST_EQUAL(buildingWay2.GetTag("addr:street"), "Main Street", ());
  TEST(buildingWay2.GetTag("wikipedia").empty(), ());

  TEST_EQUAL(buildingWay3.GetTag("addr:street"), "The Main Street", ());
  TEST_EQUAL(buildingWay3.GetTag("wikipedia"), "en:Mega Theater", ());

  TEST_EQUAL(highway4.GetTag("wikipedia"), "en:Main Street", ());
}

UNIT_TEST(RelationTags_GoodBoundary)
{
  classificator::Load();

  // Create relation.
  std::vector<RelationElement::Member> ways = {{1, "outer"}};
  std::vector<RelationElement::Member> nodes = {{2, "admin_centre"}, {3, "label"}};

  auto way1 = MakeOsmElement(1, {{"boundary", "administrative"}}, OsmElement::EntityType::Way);
  auto node2 =
      MakeOsmElement(2, {{"place", "town"}, {"name", "Vaduz"}, {"wikidata", "Q1844"}}, OsmElement::EntityType::Node);
  auto node3 = MakeOsmElement(3, {{"place", "country"}}, OsmElement::EntityType::Node);

  RelationElement e1;
  e1.m_ways = ways;
  e1.m_nodes = nodes;
  e1.m_tags.emplace("type", "boundary");
  e1.m_tags.emplace("boundary", "administrative");
  e1.m_tags.emplace("admin_level", "2");
  e1.m_tags.emplace("name", "Liechtenstein");
  e1.m_tags.emplace("name:be", "Лiхтэнштэйн");
  e1.m_tags.emplace("wikidata", "Q347");

  std::unordered_map<Key, RelationElement> m_IdToRelation = {{1, e1}};
  TestOSMElementCacheReader reader(m_IdToRelation);

  // Process ways tags using relation tags.
  RelationTagsWay rtw;

  rtw.Reset(1, &way1);
  rtw(1, reader);

  rtw.Reset(2, &node2);
  rtw(1, reader);

  rtw.Reset(3, &node3);
  rtw(1, reader);

  TEST(!way1.HasTag("name"), ());
  TEST(!way1.HasTag("name:be"), ());
  TEST(way1.GetTag("wikidata").empty(), ());

  TEST_EQUAL(node2.GetTag("place"), "town", ());
  TEST_EQUAL(node2.GetTag("name"), "Vaduz", ());
  TEST(!node2.HasTag("name:be"), ());
  TEST_EQUAL(node2.GetTag("wikidata"), "Q1844", ());

  /// @todo Take name for places?
  TEST_EQUAL(node3.GetTag("place"), "country", ());
  TEST(!node3.HasTag("name"), ());
  TEST(!node3.HasTag("name:be"), ());
  TEST_EQUAL(node3.GetTag("wikidata"), "Q347", ());
}

UNIT_TEST(RelationTags_BadBoundary)
{
  classificator::Load();

  // Create relation.
  std::vector<RelationElement::Member> testMembers = {{5, "outer"}, {6, "outer"}, {7, "outer"}};

  /// @todo Worth to add natural=peninsula Point type.
  RelationElement e1;
  e1.m_ways = testMembers;
  e1.m_tags.emplace("type", "boundary");
  e1.m_tags.emplace("boundary", "land_area");
  e1.m_tags.emplace("natural", "peninsula");
  e1.m_tags.emplace("name", "Penisola italiana");
  e1.m_tags.emplace("name:en", "Italian Peninsula");
  e1.m_tags.emplace("wikidata", "Q145694");

  std::unordered_map<Key, RelationElement> m_IdToRelation = {{1, e1}};
  TestOSMElementCacheReader reader(m_IdToRelation);

  // Create ways.
  auto outerWay5 = MakeOsmElement(5, {{"natural", "coastline"}}, OsmElement::EntityType::Way);
  auto outerWay6 = MakeOsmElement(6, {{"natural", "coastline"}, {"name", "Cala Rossa"}}, OsmElement::EntityType::Way);
  auto outerWay7 = MakeOsmElement(7, {{"place", "locality"}}, OsmElement::EntityType::Way);

  // Process ways tags using relation tags.
  RelationTagsWay rtw;

  rtw.Reset(5, &outerWay5);
  rtw(1, reader);

  rtw.Reset(6, &outerWay6);
  rtw(1, reader);

  rtw.Reset(7, &outerWay7);
  rtw(1, reader);

  // We don't aggregate name and wiki from type=boundary Relation if destination Way (Node) is not a place.
  TEST(!outerWay5.HasTag("place"), ());
  TEST(!outerWay5.HasTag("name"), ());
  TEST(!outerWay5.HasTag("name:en"), ());
  TEST(outerWay5.GetTag("wikidata").empty(), ());

  TEST(!outerWay6.HasTag("place"), ());
  TEST_EQUAL(outerWay6.GetTag("name"), "Cala Rossa", ());
  TEST(!outerWay6.HasTag("name:en"), ());
  TEST(outerWay6.GetTag("wikidata").empty(), ());

  // Process only boundary=* valid classifier Relations.
  TEST_EQUAL(outerWay7.GetTag("place"), "locality", ());
  TEST(!outerWay7.HasTag("name"), ());
  TEST(!outerWay7.HasTag("name:en"), ());
  TEST(outerWay7.GetTag("wikidata").empty(), ());
}

}  // namespace relation_tags_tests
