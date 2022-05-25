#include "testing/testing.hpp"

#include "generator/generator_tests/common.hpp"
#include "generator/relation_tags.hpp"


namespace relation_tags_tests
{
using namespace generator_tests;
using namespace generator;
using namespace feature;
using namespace std;

// In memory relations storage (copy-n-paste from collector_building_parts_tests.cpp).
class TestOSMElementCacheReader : public generator::cache::OSMElementCacheReaderInterface
{
public:
  TestOSMElementCacheReader(std::unordered_map<generator::cache::Key, RelationElement> & m)
    : m_mapping(m)
  {
  }

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


UNIT_TEST() {
  /* Prepare relations data:
   * Relation 1:
   *   - type = associatedStreet
   *   - name = Main street
   *   - wikipedia = en:Main Street
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
  std::vector<RelationElement::Member> testMembers = {{2, "house"},
                                                      {3, "house"}};

  RelationElement e1;
  e1.m_ways = testMembers;
  e1.m_tags.emplace("type", "associatedStreet");
  e1.m_tags.emplace("name", "Main Street");
  e1.m_tags.emplace("wikipedia", "en:Main Street");

  std::unordered_map<generator::cache::Key, RelationElement> m_IdToRelation = {{1, e1}};
  TestOSMElementCacheReader reader(m_IdToRelation);

  // Create buildings polygons.
  auto buildingWay2 = MakeOsmElement(2, {{"building", "yes"},
    {"addr:housenumber", "121"}},
  OsmElement::EntityType::Way);

  auto buildingWay3 = MakeOsmElement(3, {{"auto const", "yes"},
    {"addr:housenumber", "123"},
    {"addr:street", "The Main Street"},
    {"wikipedia", "en:Mega Theater"}},
  OsmElement::EntityType::Way);

  // Process way tags using relation tags.
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

}