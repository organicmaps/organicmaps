#include "../../testing/testing.hpp"

#include "../osm_id.hpp"
#include "../feature_builder.hpp"

#include "../../base/logging.hpp"

using namespace osm;

UNIT_TEST(OsmId)
{
  OsmId node("node", 12345);
  TEST_EQUAL(node.Id(), 12345ULL, ());
  TEST_EQUAL(node.Type(), "node", ());

  OsmId way("way", 93245123456332ULL);
  TEST_EQUAL(way.Id(), 93245123456332ULL, ());
  TEST_EQUAL(way.Type(), "way", ());

  OsmId relation("relation", 5);
  TEST_EQUAL(relation.Id(), 5ULL, ());
  TEST_EQUAL(relation.Type(), "relation", ());
}
