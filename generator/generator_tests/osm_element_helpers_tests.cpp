#include "testing/testing.hpp"

#include "generator/osm_element_helpers.hpp"

#include "geometry/point2d.hpp"

namespace
{
using namespace generator::osm_element;

OsmElement CreateOsmElement(std::string const & populationStr)
{
  OsmElement element;
  element.m_tags.emplace_back("population", populationStr);
  return element;
}

UNIT_TEST(ParsePopulation)
{
  TEST_EQUAL(GetPopulation(CreateOsmElement("123")), 123, ());
  TEST_EQUAL(GetPopulation(CreateOsmElement("123 123")), 123123, ());
  TEST_EQUAL(GetPopulation(CreateOsmElement("12,300")), 12300, ());
  TEST_EQUAL(GetPopulation(CreateOsmElement("000")), 0, ());
  TEST_EQUAL(GetPopulation(CreateOsmElement("123.321")), 123321, ());
  TEST_EQUAL(GetPopulation(CreateOsmElement("123 000 000 (Moscow Info)")), 123000000, ());
  TEST_EQUAL(GetPopulation(CreateOsmElement("123 000 000 (123)")), 123000000, ());
  TEST_EQUAL(GetPopulation(CreateOsmElement("")), 0, ());
  TEST_EQUAL(GetPopulation(CreateOsmElement("  ")), 0, ());
  TEST_EQUAL(GetPopulation(CreateOsmElement("asd")), 0, ());
  TEST_EQUAL(GetPopulation(CreateOsmElement("sfa843r")), 0, ());
  TEST_EQUAL(GetPopulation(OsmElement()), 0, ());
}
}  // namespace
