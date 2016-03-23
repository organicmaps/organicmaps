#include "testing/testing.hpp"

#include "generator/tag_admixer.hpp"

#include "std/map.hpp"
#include "std/set.hpp"
#include "std/sstream.hpp"

UNIT_TEST(WaysParserTests)
{
  map<uint64_t, string> ways;
  WaysParserHelper parser(ways);
  istringstream stream("140247102;world_level\n86398306;another_level\n294584441;world_level");
  parser.ParseStream(stream);
  TEST(ways.find(140247102) != ways.end(), ());
  TEST_EQUAL(ways[140247102], string("world_level"), ());
  TEST(ways.find(86398306) != ways.end(), ());
  TEST_EQUAL(ways[86398306], string("another_level"), ());
  TEST(ways.find(294584441) != ways.end(), ());
  TEST_EQUAL(ways[294584441], string("world_level"), ());
  TEST(ways.find(140247101) == ways.end(), ());
}

UNIT_TEST(CapitalsParserTests)
{
  set<uint64_t> capitals;
  CapitalsParserHelper parser(capitals);
  istringstream stream("-21.1343401;-175.201808;1082208696;t\n-16.6934156;-179.87995;242715809;f\n19.0534159;169.919199;448768937;t");
  parser.ParseStream(stream);
  TEST(capitals.find(1082208696) != capitals.end(), ());
  TEST(capitals.find(242715809) != capitals.end(), ());
  TEST(capitals.find(448768937) != capitals.end(), ());
  TEST(capitals.find(140247101) == capitals.end(), ());
}
