#include "testing/testing.hpp"

#include "generator/generator_tests/source_data.hpp"
#include "generator/osm_element.hpp"
#include "generator/osm_source.hpp"

#include "coding/parse_xml.hpp"

#include <cstddef>
#include <iostream>
#include <iterator>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace generator;

UNIT_TEST(Source_To_Element_create_from_xml_test)
{
  std::istringstream ss(way_xml_data);
  SourceReader reader(ss);

  std::vector<OsmElement> elements;
  ProcessOsmElementsFromXML(reader, [&elements](OsmElement && e) { elements.push_back(std::move(e)); });

  TEST_EQUAL(elements.size(), 10, (elements));
}

UNIT_TEST(Source_To_Element_Negative_Xml_Ids)
{
  std::istringstream ss(R"(<osm>
    <node id="-1" lat="1" lon="2"/>
    <way id="-2"><nd ref="-1"/></way>
    <relation id="-3"><member type="way" ref="-2" role="outer"/></relation>
  </osm>)");
  SourceReader reader(ss);
  std::vector<OsmElement> elements;
  ProcessOsmElementsFromXML(reader, [&elements](OsmElement && e) { elements.push_back(std::move(e)); });

  uint64_t constexpr kNegativeOne = std::numeric_limits<uint64_t>::max();
  TEST_EQUAL(elements.size(), 3, ());
  TEST_EQUAL(elements[0].m_id, kNegativeOne, ());
  TEST_EQUAL(elements[1].m_id, kNegativeOne - 1, ());
  TEST_EQUAL(elements[1].m_nodes, std::vector<uint64_t>{kNegativeOne}, ());
  TEST_EQUAL(elements[2].m_id, kNegativeOne - 2, ());
  TEST_EQUAL(elements[2].m_members.size(), 1, ());
  TEST_EQUAL(elements[2].m_members[0].m_ref, kNegativeOne - 1, ());
  TEST_EQUAL(elements[2].m_members[0].m_type, OsmElement::EntityType::Way, ());
}

UNIT_TEST(Source_To_Element_create_from_o5m_test)
{
  std::string src(std::begin(relation_o5m_data), std::end(relation_o5m_data));
  std::istringstream ss(src);
  SourceReader reader(ss);

  std::vector<OsmElement> elements;
  ProcessOsmElementsFromO5M(reader, [&elements](OsmElement && e) { elements.push_back(std::move(e)); });
  TEST_EQUAL(elements.size(), 11, (elements));

  std::cout << DebugPrint(elements);
}

UNIT_TEST(Source_To_Element_check_equivalence)
{
  std::istringstream ss1(relation_xml_data);
  SourceReader readerXML(ss1);

  std::vector<OsmElement> elementsXML;
  ProcessOsmElementsFromXML(readerXML, [&elementsXML](OsmElement && e) { elementsXML.push_back(std::move(e)); });

  std::string src(std::begin(relation_o5m_data), std::end(relation_o5m_data));
  std::istringstream ss2(src);
  SourceReader readerO5M(ss2);

  std::vector<OsmElement> elementsO5M;
  ProcessOsmElementsFromO5M(readerO5M, [&elementsO5M](OsmElement && e) { elementsO5M.push_back(std::move(e)); });

  TEST_EQUAL(elementsXML.size(), elementsO5M.size(), ());

  for (size_t i = 0; i < elementsO5M.size(); ++i)
    TEST_EQUAL(elementsXML[i], elementsO5M[i], ());
}
