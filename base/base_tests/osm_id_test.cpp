#include "testing/testing.hpp"

#include "base/osm_id.hpp"

namespace osm
{
UNIT_TEST(OsmId)
{
  Id const node = Id::Node(12345);
  TEST_EQUAL(node.GetOsmId(), 12345ULL, ());
  TEST_EQUAL(node.GetType(), Id::Type::Node, ());
  TEST_EQUAL(DebugPrint(node), "node 12345", ());

  Id const way = Id::Way(93245123456332ULL);
  TEST_EQUAL(way.GetOsmId(), 93245123456332ULL, ());
  TEST_EQUAL(way.GetType(), Id::Type::Way, ());
  TEST_EQUAL(DebugPrint(way), "way 93245123456332", ());

  Id const relation = Id::Relation(5);
  TEST_EQUAL(relation.GetOsmId(), 5ULL, ());
  TEST_EQUAL(relation.GetType(), Id::Type::Relation, ());
  TEST_EQUAL(DebugPrint(relation), "relation 5", ());
}
}  // namespace osm
