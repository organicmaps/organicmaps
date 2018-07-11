#include "testing/testing.hpp"

#include "base/osm_id.hpp"

using namespace osm;

UNIT_TEST(OsmId)
{
  Id const node = Id::Node(12345);
  TEST_EQUAL(node.OsmId(), 12345ULL, ());
  TEST(node.IsNode(), ());
  TEST(!node.IsWay(), ());
  TEST(!node.IsRelation(), ());
  TEST_EQUAL(DebugPrint(node), "node 12345", ());

  Id const way = Id::Way(93245123456332ULL);
  TEST_EQUAL(way.OsmId(), 93245123456332ULL, ());
  TEST(!way.IsNode(), ());
  TEST(way.IsWay(), ());
  TEST(!way.IsRelation(), ());
  TEST_EQUAL(DebugPrint(way), "way 93245123456332", ());

  Id const relation = Id::Relation(5);
  TEST_EQUAL(relation.OsmId(), 5ULL, ());
  // sic!
  TEST(relation.IsNode(), ());
  TEST(relation.IsWay(), ());
  TEST(relation.IsRelation(), ());
  TEST_EQUAL(DebugPrint(relation), "relation 5", ());
}
