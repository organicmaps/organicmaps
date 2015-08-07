//
//  source_to_element_test.cpp
//  generator_tool
//
//  Created by Sergey Yershov on 05.08.15.
//  Copyright (c) 2015 maps.me. All rights reserved.
//

#include "testing/testing.hpp"

#include "coding/parse_xml.hpp"
#include "generator/osm_source.hpp"
#include "generator/xml_element.hpp"

#include "source_data.hpp"

struct DummyParser : public BaseOSMParser
{
  vector<XMLElement> & m_e;
  DummyParser(vector<XMLElement> & e) : BaseOSMParser() , m_e(e) {}
  void EmitElement(XMLElement * p) override
  {
    m_e.push_back(*p);
  }
};

UNIT_TEST(Source_To_Element_create_from_xml_test)
{
  istringstream ss(way_xml_data);
  SourceReader reader(ss);

  vector<XMLElement> elements;
  DummyParser parser(elements);
  ParseXMLSequence(reader, parser);

  TEST_EQUAL(elements.size(), 10, (elements));
}

UNIT_TEST(Source_To_Element_create_from_o5m_test)
{
  string src(begin(way_o5m_data), end(way_o5m_data));
  istringstream ss(src);
  SourceReader reader(ss);

  vector<XMLElement> elements;
  DummyParser parser(elements);
  BuildFeaturesFromO5M(reader, parser);
  TEST_EQUAL(elements.size(), 10, (elements));
}

UNIT_TEST(Source_To_Element_check_equivalence)
{
  istringstream ss1(relation_xml_data);
  SourceReader readerXML(ss1);

  vector<XMLElement> elementsXML;
  DummyParser parserForXML(elementsXML);
  ParseXMLSequence(readerXML, parserForXML);

  string src(begin(relation_o5m_data), end(relation_o5m_data));
  istringstream ss2(src);
  SourceReader readerO5M(ss2);

  vector<XMLElement> elementsO5M;
  DummyParser parserForO5M(elementsO5M);
  BuildFeaturesFromO5M(readerO5M, parserForO5M);

  TEST_EQUAL(elementsXML, elementsO5M, ());
}
