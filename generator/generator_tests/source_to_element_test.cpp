#include "testing/testing.hpp"

#include "coding/parse_xml.hpp"
#include "generator/osm_source.hpp"
#include "generator/osm_element.hpp"

#include "std/iterator.hpp"

#include "source_data.hpp"

UNIT_TEST(Source_To_Element_create_from_xml_test)
{
  istringstream ss(way_xml_data);
  SourceReader reader(ss);

  vector<OsmElement> elements;
  ProcessOsmElementsFromXML(reader, [&elements](OsmElement * e)
  {
    elements.push_back(*e);
  });

  TEST_EQUAL(elements.size(), 10, (elements));
}

UNIT_TEST(Source_To_Element_create_from_o5m_test)
{
  string src(begin(relation_o5m_data), end(relation_o5m_data));
  istringstream ss(src);
  SourceReader reader(ss);

  vector<OsmElement> elements;
  ProcessOsmElementsFromO5M(reader, [&elements](OsmElement * e)
  {
    elements.push_back(*e);
  });
  TEST_EQUAL(elements.size(), 11, (elements));

  cout << DebugPrint(elements);
}

UNIT_TEST(Source_To_Element_check_equivalence)
{
  istringstream ss1(relation_xml_data);
  SourceReader readerXML(ss1);

  vector<OsmElement> elementsXML;
  ProcessOsmElementsFromXML(readerXML, [&elementsXML](OsmElement * e)
  {
    elementsXML.push_back(*e);
  });

  string src(begin(relation_o5m_data), end(relation_o5m_data));
  istringstream ss2(src);
  SourceReader readerO5M(ss2);

  vector<OsmElement> elementsO5M;
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
