#include "testing/testing.hpp"

#include "indexer/osm_value_format.hpp"

#include <array>
#include <string>
#include <utility>

UNIT_TEST(OsmValueFormat_building_levels)
{
  TEST_EQUAL(osm::ValidateAndFormat_building_levels("４"), "4", ());
  TEST_EQUAL(osm::ValidateAndFormat_building_levels("４floors"), "4", ());
  TEST_EQUAL(osm::ValidateAndFormat_building_levels("between 1 and ４"), "", ());
  TEST_EQUAL(osm::ValidateAndFormat_building_levels("0"), "0", ("OSM has many zero-level buildings."));
  TEST_EQUAL(osm::ValidateAndFormat_building_levels("0.0"), "0", ());
  TEST_EQUAL(osm::ValidateAndFormat_building_levels(""), "", ());
  TEST_EQUAL(osm::ValidateAndFormat_building_levels("Level 1"), "", ());
  TEST_EQUAL(osm::ValidateAndFormat_building_levels("2.51"), "2.5", ());
  TEST_EQUAL(osm::ValidateAndFormat_building_levels("250"), "", ("Too many levels."));
}

UNIT_TEST(OsmValueFormat_url)
{
  std::array<std::pair<char const *, char const *>, 9> constexpr kTests = {{
      {"a.by", "a.by"},
      {"http://test.com", "http://test.com"},
      {"https://test.com", "https://test.com"},
      {"test.com", "test.com"},
      {"http://test.com/", "http://test.com"},
      {"https://test.com/", "https://test.com"},
      {"test.com/", "test.com"},
      {"test.com/path", "test.com/path"},
      {"test.com/path/", "test.com/path/"},
  }};

  for (auto const & [input, output] : kTests)
    TEST_EQUAL(osm::ValidateAndFormat_url(input), output, ());
}

UNIT_TEST(OsmValueFormat_stars)
{
  TEST_EQUAL(osm::ValidateAndFormat_stars(""), "", ());
  TEST_EQUAL(osm::ValidateAndFormat_stars("0"), "", ());
  TEST_EQUAL(osm::ValidateAndFormat_stars("5"), "5", ());
  TEST_EQUAL(osm::ValidateAndFormat_stars("7"), "7", ());
  TEST_EQUAL(osm::ValidateAndFormat_stars("8"), "", ());
  TEST_EQUAL(osm::ValidateAndFormat_stars("55"), "", ("A second digit means a larger number."));
  TEST_EQUAL(osm::ValidateAndFormat_stars("5S"), "5", ("Superior rating, the star count still counts."));
  TEST_EQUAL(osm::ValidateAndFormat_stars("5\xEF\xBC\x95"), "5", ("A fullwidth digit is not a digit here."));
}

UNIT_TEST(OsmValueFormat_NormalizeCuisineToken)
{
  TEST_EQUAL(osm::NormalizeCuisineToken(""), "", ());
  TEST_EQUAL(osm::NormalizeCuisineToken("   "), "", ());
  TEST_EQUAL(osm::NormalizeCuisineToken("  Pizza  "), "pizza", ());
  TEST_EQUAL(osm::NormalizeCuisineToken("ICE   CREAM"), "ice_cream", ());
  TEST_EQUAL(osm::NormalizeCuisineToken("BBQ"), "barbecue", ());
  TEST_EQUAL(osm::NormalizeCuisineToken("barbeque"), "barbecue", ());
  TEST_EQUAL(osm::NormalizeCuisineToken("doughnut"), "donut", ());
  TEST_EQUAL(osm::NormalizeCuisineToken("Steak"), "steak_house", ());
  TEST_EQUAL(osm::NormalizeCuisineToken("coffee"), "coffee_shop", ());
  TEST_EQUAL(osm::NormalizeCuisineToken(osm::NormalizeCuisineToken("  BBQ ")), "barbecue", ("Idempotent."));
}
