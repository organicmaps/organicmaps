//
//  intermediate_data_test.cpp
//  generator_tool
//
//  Created by Sergey Yershov on 20.08.15.
//  Copyright (c) 2015 maps.me. All rights reserved.
//

#include "testing/testing.hpp"

#include "generator/intermediate_elements.hpp"


UNIT_TEST(Intermediate_Data_empty_way_element_save_load_test)
{
  WayElement e1(1 /* fake osm id */);

  using TBuffer = vector<uint8_t>;
  TBuffer buffer;
  MemWriter<TBuffer> w(buffer);

  e1.Write(w);

  MemReader r(buffer.data(), buffer.size());

  WayElement e2(1 /* fake osm id */);

  e2.Read(r);

  TEST_EQUAL(e2.nodes.size(), 0, ());
}


UNIT_TEST(Intermediate_Data_way_element_save_load_test)
{
  WayElement e1(1 /* fake osm id */);

  e1.nodes.push_back(0);
  e1.nodes.push_back(1);
  e1.nodes.push_back(2);
  e1.nodes.push_back(3);
  e1.nodes.push_back(0xFFFFFFFF);
  e1.nodes.push_back(0xFFFFFFFFFFFFFFFF);

  using TBuffer = vector<uint8_t>;
  TBuffer buffer;
  MemWriter<TBuffer> w(buffer);

  e1.Write(w);

  MemReader r(buffer.data(), buffer.size());

  WayElement e2(1 /* fake osm id */);

  e2.Read(r);

  TEST_EQUAL(e2.nodes.size(), 6, ());
  TEST_EQUAL(e2.nodes[0], 0, ());
  TEST_EQUAL(e2.nodes[1], 1, ());
  TEST_EQUAL(e2.nodes[2], 2, ());
  TEST_EQUAL(e2.nodes[3], 3, ());
  TEST_EQUAL(e2.nodes[4], 0xFFFFFFFF, ());
  TEST_EQUAL(e2.nodes[5], 0xFFFFFFFFFFFFFFFF, ());
}

UNIT_TEST(Intermediate_Data_relation_element_save_load_test)
{
  RelationElement e1;

  e1.nodes.emplace_back(1, "inner");
  e1.nodes.emplace_back(2, "outer");
  e1.nodes.emplace_back(3, "unknown");
  e1.nodes.emplace_back(4, "inner role");

  e1.ways.emplace_back(1, "inner");
  e1.ways.emplace_back(2, "outer");
  e1.ways.emplace_back(3, "unknown");
  e1.ways.emplace_back(4, "inner role");

  e1.tags.emplace("key1","value1");
  e1.tags.emplace("key2","value2");
  e1.tags.emplace("key3","value3");
  e1.tags.emplace("key4","value4");

  using TBuffer = vector<uint8_t>;
  TBuffer buffer;
  MemWriter<TBuffer> w(buffer);

  e1.Write(w);

  MemReader r(buffer.data(), buffer.size());

  RelationElement e2;

  e2.nodes.emplace_back(30, "000unknown");
  e2.nodes.emplace_back(40, "000inner role");
  e2.ways.emplace_back(10, "000inner");
  e2.ways.emplace_back(20, "000outer");
  e2.tags.emplace("key1old","value1old");
  e2.tags.emplace("key2old","value2old");

  e2.Read(r);

  TEST_EQUAL(e2.nodes.size(), 4, ());
  TEST_EQUAL(e2.ways.size(), 4, ());
  TEST_EQUAL(e2.tags.size(), 4, ());

  TEST_EQUAL(e2.nodes[0].first, 1, ());
  TEST_EQUAL(e2.nodes[1].first, 2, ());
  TEST_EQUAL(e2.nodes[2].first, 3, ());
  TEST_EQUAL(e2.nodes[3].first, 4, ());

  TEST_EQUAL(e2.nodes[0].second, "inner", ());
  TEST_EQUAL(e2.nodes[1].second, "outer", ());
  TEST_EQUAL(e2.nodes[2].second, "unknown", ());
  TEST_EQUAL(e2.nodes[3].second, "inner role", ());

  TEST_EQUAL(e2.ways[0].first, 1, ());
  TEST_EQUAL(e2.ways[1].first, 2, ());
  TEST_EQUAL(e2.ways[2].first, 3, ());
  TEST_EQUAL(e2.ways[3].first, 4, ());

  TEST_EQUAL(e2.ways[0].second, "inner", ());
  TEST_EQUAL(e2.ways[1].second, "outer", ());
  TEST_EQUAL(e2.ways[2].second, "unknown", ());
  TEST_EQUAL(e2.ways[3].second, "inner role", ());

  TEST_EQUAL(e2.tags["key1"], "value1", ());
  TEST_EQUAL(e2.tags["key2"], "value2", ());
  TEST_EQUAL(e2.tags["key3"], "value3", ());
  TEST_EQUAL(e2.tags["key4"], "value4", ());

  TEST_NOT_EQUAL(e2.tags["key1old"], "value1old", ());
  TEST_NOT_EQUAL(e2.tags["key2old"], "value2old", ());
}
