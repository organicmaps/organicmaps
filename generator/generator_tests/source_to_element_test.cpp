#include "testing/testing.hpp"

#include "coding/parse_xml.hpp"
#include "generator/osm_source.hpp"
#include "generator/osm_element.hpp"

#include <iterator>

#include "source_data.hpp"

UNIT_TEST(Source_To_Element_create_from_xml_test)
{
  std::istringstream ss(way_xml_data);
  SourceReader reader(ss);

  std::vector<OsmElement> elements;
  ProcessOsmElementsFromXML(reader, [&elements](OsmElement * e)
  {
    elements.push_back(*e);
  });

  TEST_EQUAL(elements.size(), 10, (elements));
}

UNIT_TEST(Source_To_Element_create_from_o5m_test)
{
  std::string src(std::begin(relation_o5m_data), std::end(relation_o5m_data));
  std::istringstream ss(src);
  SourceReader reader(ss);

  std::vector<OsmElement> elements;
  ProcessOsmElementsFromO5M(reader, [&elements](OsmElement * e)
  {
    elements.push_back(*e);
  });
  TEST_EQUAL(elements.size(), 11, (elements));

  std::cout << DebugPrint(elements);
}

UNIT_TEST(Source_To_Element_check_equivalence)
{
  std::istringstream ss1(relation_xml_data);
  SourceReader readerXML(ss1);

  std::vector<OsmElement> elementsXML;
  ProcessOsmElementsFromXML(readerXML, [&elementsXML](OsmElement * e)
  {
    elementsXML.push_back(*e);
  });

  std::string src(std::begin(relation_o5m_data), std::end(relation_o5m_data));
  std::istringstream ss2(src);
  SourceReader readerO5M(ss2);

  std::vector<OsmElement> elementsO5M;
  ProcessOsmElementsFromO5M(readerO5M, [&elementsO5M](OsmElement * e)
  {
    elementsO5M.push_back(*e);
  });

  TEST_EQUAL(elementsXML.size(), elementsO5M.size(), ());

  for (size_t i = 0; i < elementsO5M.size(); ++i)
  {
    TEST_EQUAL(elementsXML[i], elementsO5M[i], ());
  }
}
