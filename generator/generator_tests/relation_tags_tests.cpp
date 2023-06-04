#include "testing/testing.hpp"

#include "generator/generator_tests/common.hpp"
#include "generator/relation_tags.hpp"
#include "generator/intermediate_data.hpp"

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
  TestOSMElementCacheReader(std::unordered_map<Key, RelationElement> & m)
    : m_mapping(m)
  {
  }

  // OSMElementCacheReaderInterface overrides:
  bool Read(Key /* id */, WayElement & /* value */) override
  {
    UNREACHABLE();
  }

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
  TEST_EQUAL(road11.GetTag("ref"), "SP62", ()); // TODO: Check refs inheritance (expected "IT:RA/SP60;SP62")
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
  std::vector<RelationElement::Member> testMembers = {{2, "house"}, {3, "house"}};

  RelationElement e1;
  e1.m_ways = testMembers;
  e1.m_tags.emplace("type", "associatedStreet");
  e1.m_tags.emplace("name", "Main Street");
  e1.m_tags.emplace("wikipedia", "en:Main Street");

  std::unordered_map<Key, RelationElement> m_IdToRelation = {{1, e1}};
  TestOSMElementCacheReader reader(m_IdToRelation);

  // Create buildings polygons.
  auto buildingWay2 = MakeOsmElement(2, {{"building", "yes"}, {"addr:housenumber", "121"}}, OsmElement::EntityType::Way);
  auto buildingWay3 = MakeOsmElement(3, {{"auto const", "yes"}, {"addr:housenumber", "123"}, {"addr:street", "The Main Street"}, {"wikipedia", "en:Mega Theater"}}, OsmElement::EntityType::Way);

  // Process buildings tags using relation tags.
  RelationTagsWay rtw;

  rtw.Reset(2, &buildingWay2);
  rtw(1, reader);

  rtw.Reset(3, &buildingWay3);
  rtw(1, reader);

  // Verify ways tags.
  TEST_EQUAL(buildingWay2.GetTag("addr:street"), "Main Street", ());
  TEST_EQUAL(buildingWay2.HasTag("wikipedia"), false, ());

  TEST_EQUAL(buildingWay3.GetTag("addr:street"), "The Main Street", ());
  TEST_EQUAL(buildingWay3.HasTag("wikipedia"), true, ());
}

UNIT_TEST(Process_boundary)
{
  /* Prepare relations data:
   * Relation 1:
   *   - type = boundary
   *   - place = peninsula
   *   - name = Penisola italiana
   *   - members: [
   *       Way 5:
   *         - natural = coastline
   *       Way 6:
   *         - natural = coastline
   *         - name = Cala Rossa
   *     ]
   */

  // Create relation.
  std::vector<RelationElement::Member> testMembers = {{5, "outer"}, {6, "outer"}};

  RelationElement e1;
  e1.m_ways = testMembers;
  e1.m_tags.emplace("type", "boundary");
  e1.m_tags.emplace("place", "peninsula");
  e1.m_tags.emplace("name", "Penisola italiana");
  e1.m_tags.emplace("name:en", "Italian Peninsula");
  e1.m_tags.emplace("wikidata", "Q145694");

  std::unordered_map<Key, RelationElement> m_IdToRelation = {{1, e1}};
  TestOSMElementCacheReader reader(m_IdToRelation);

  // Create ways.
  auto outerWay5 = MakeOsmElement(5, {{"natural", "coastline"}}, OsmElement::EntityType::Way);
  auto outerWay6 = MakeOsmElement(6, {{"natural", "coastline"}, {"name", "Cala Rossa"}}, OsmElement::EntityType::Way);

  // Process ways tags using relation tags.
  RelationTagsWay rtw;

  rtw.Reset(5, &outerWay5);
  rtw(1, reader);

  rtw.Reset(6, &outerWay6);
  rtw(1, reader);

  // Verify ways tags.
  TEST_EQUAL(outerWay5.HasTag("place"), false, ());
  TEST_EQUAL(outerWay5.HasTag("name"), false, ());
  TEST_EQUAL(outerWay5.HasTag("name:en"), false, ());
  TEST_EQUAL(outerWay5.GetTag("wikidata"), "Q145694", ());

  TEST_EQUAL(outerWay6.HasTag("place"), false, ());
  TEST_EQUAL(outerWay6.HasTag("name"), true, ());
  TEST_EQUAL(outerWay6.HasTag("name:en"), false, ());
  TEST_EQUAL(outerWay6.GetTag("wikidata"), "Q145694", ());
}

} // namespace relation_tags_tests
