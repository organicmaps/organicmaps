#include "testing/testing.hpp"

#include "generator/tag_admixer.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <string>

using platform::tests_support::ScopedFile;

namespace
{
void TestReplacer(std::string const & source,
                  std::map<TagReplacer::Tag, std::vector<TagReplacer::Tag>> const & expected)
{
  auto const filename = "test.txt";
  ScopedFile sf(filename, source);
  TagReplacer replacer(sf.GetFullPath());

  for (auto const & r : expected)
  {
    auto const * tags = replacer.GetTagsForTests(r.first);
    TEST(tags, ());
    TEST_EQUAL(*tags, r.second, ());
  }
}
}  // namespace

UNIT_TEST(WaysParserTests)
{
  std::map<uint64_t, std::string> ways;
  WaysParserHelper parser(ways);
  std::istringstream stream("140247102;world_level\n86398306;another_level\n294584441;world_level");
  parser.ParseStream(stream);
  TEST(ways.find(140247102) != ways.end(), ());
  TEST_EQUAL(ways[140247102], std::string("world_level"), ());
  TEST(ways.find(86398306) != ways.end(), ());
  TEST_EQUAL(ways[86398306], std::string("another_level"), ());
  TEST(ways.find(294584441) != ways.end(), ());
  TEST_EQUAL(ways[294584441], std::string("world_level"), ());
  TEST(ways.find(140247101) == ways.end(), ());
}

UNIT_TEST(CapitalsParserTests)
{
  std::set<uint64_t> capitals;
  CapitalsParserHelper parser(capitals);
  std::istringstream stream("-21.1343401;-175.201808;1082208696;t\n-16.6934156;-179.87995;242715809;f\n19.0534159;169.919199;448768937;t");
  parser.ParseStream(stream);
  TEST(capitals.find(1082208696) != capitals.end(), ());
  TEST(capitals.find(242715809) != capitals.end(), ());
  TEST(capitals.find(448768937) != capitals.end(), ());
  TEST(capitals.find(140247101) == capitals.end(), ());
}

UNIT_TEST(TagsReplacer_Smoke)
{
  {
    std::string const source = "";
    TestReplacer(source, {});
  }
  {
    std::string const source = "aerodrome:type=international : aerodrome=international";
    TestReplacer(source, {{{"aerodrome:type", "international"}, {{"aerodrome", "international"}}}});
  }
  {
    std::string const source =
        "  aerodrome:type   =   international   :    aerodrome   =  international   ";
    TestReplacer(source, {{{"aerodrome:type", "international"}, {{"aerodrome", "international"}}}});
  }
  {
    std::string const source = "natural=marsh : natural=wetland, wetland=marsh";
    TestReplacer(source, {{{"natural", "marsh"}, {{"natural", "wetland"}, {"wetland", "marsh"}}}});
  }
  {
    std::string const source =
        "natural = forest : natural = wood\n"
        "# TODO\n"
        "cliff=yes : natural=cliff | u\n"
        "\n"
        "office=travel_agent : shop=travel_agency";
    TestReplacer(source, {
        {{"cliff", "yes"}, {{"natural", "cliff"}}},
        {{"natural", "forest"}, {{"natural", "wood"}}},
        {{"office", "travel_agent"}, {{"shop", "travel_agency"}}}});
  }
}
