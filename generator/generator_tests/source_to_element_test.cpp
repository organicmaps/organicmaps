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


char const node_xml_data[] = "<?xml version='1.0' encoding='UTF-8'?> \
 <osm version='0.6' upload='true' generator='JOSM'> \
 <node id='-273105' action='modify' visible='true' lat='62.18269750679' lon='-134.28965517091'> \
 <tag k='name' v='Продуктовый' /> \
 <tag k='opening_hours' v='24/7' /> \
 <tag k='shop' v='convenience' /> \
 </node> \
 </osm> \
";

// binary data: node.o5m
uint8_t node_o5m_data1[] = /* 92 */
{0xFF, 0xE0, 0x04, 0x6F, 0x35, 0x6D, 0x32, 0xFF, 0x10, 0x51, 0xA1, 0xAB, 0x21, 0x00, 0xCD, 0xE6,
  0xD7, 0x80, 0x0A, 0xBE, 0xCE, 0x82, 0xD1, 0x04, 0x00, 0x6E, 0x61, 0x6D, 0x65, 0x00, 0xD0, 0x9F,
  0xD1, 0x80, 0xD0, 0xBE, 0xD0, 0xB4, 0xD1, 0x83, 0xD0, 0xBA, 0xD1, 0x82, 0xD0, 0xBE, 0xD0, 0xB2,
  0xD1, 0x8B, 0xD0, 0xB9, 0x00, 0x00, 0x6F, 0x70, 0x65, 0x6E, 0x69, 0x6E, 0x67, 0x5F, 0x68, 0x6F,
  0x75, 0x72, 0x73, 0x00, 0x32, 0x34, 0x2F, 0x37, 0x00, 0x00, 0x73, 0x68, 0x6F, 0x70, 0x00, 0x63,
  0x6F, 0x6E, 0x76, 0x65, 0x6E, 0x69, 0x65, 0x6E, 0x63, 0x65, 0x00, 0xFE};
static_assert(sizeof(node_o5m_data1) == 92, "Size check failed");


struct DummyParser : public BaseOSMParser
{
  XMLElement & m_e;
  DummyParser(XMLElement & e) : BaseOSMParser() , m_e(e) {}
  void EmitElement(XMLElement * p) override
  {
    m_e = *p;
  }
};

UNIT_TEST(Source_To_Element_check_equivalence)
{
  istringstream ss1(node_xml_data);
  SourceReader reader1(ss1);

  XMLElement e1;
  DummyParser parser1(e1);
  ParseXMLSequence(reader1, parser1);

  string src(begin(node_o5m_data1), end(node_o5m_data1));
  istringstream ss2(src);
  SourceReader reader2(ss2);

  XMLElement e2;
  DummyParser parser2(e2);
  BuildFeaturesFromO5M(reader2, parser2);

  TEST_EQUAL(e1, e2, ());
}


UNIT_TEST(Source_To_Element_create_from_xml_test)
{
  istringstream ss(node_xml_data);
  SourceReader reader(ss);

  XMLElement e;
  DummyParser parser(e);
  ParseXMLSequence(reader, parser);
}

UNIT_TEST(Source_To_Element_create_from_o5m_test)
{
  string src(begin(node_o5m_data1), end(node_o5m_data1));
  istringstream ss(src);
  SourceReader reader(ss);

  XMLElement e;
  DummyParser parser(e);
  BuildFeaturesFromO5M(reader, parser);
}