#include "testing/testing.hpp"

#include "generator/tag_admixer.hpp"

#include "std/map.hpp"
#include "std/sstream.hpp"

UNIT_TEST(ParserTests)
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
