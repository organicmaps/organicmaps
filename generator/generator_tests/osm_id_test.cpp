#include "testing/testing.hpp"

#include "generator/feature_builder.hpp"

#include "base/logging.hpp"
#include "base/osm_id.hpp"

using namespace osm;

UNIT_TEST(OsmId)
{
  Id const node = Id::Node(12345);
  TEST_EQUAL(node.OsmId(), 12345ULL, ());
  TEST_EQUAL(node.Type(), "node", ());

  Id const way = Id::Way(93245123456332ULL);
  TEST_EQUAL(way.OsmId(), 93245123456332ULL, ());
  TEST_EQUAL(way.Type(), "way", ());

  Id const relation = Id::Relation(5);
  TEST_EQUAL(relation.OsmId(), 5ULL, ());
  TEST_EQUAL(relation.Type(), "relation", ());
}
