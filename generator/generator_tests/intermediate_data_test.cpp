//
//  intermediate_data_test.cpp
//  generator_tool
//
//  Created by Sergey Yershov on 20.08.15.
//  Copyright (c) 2015 maps.me. All rights reserved.
//

#include "testing/testing.hpp"

#include "generator/intermediate_elements.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace intermediate_data_test
{
UNIT_TEST(Intermediate_Data_empty_way_element_save_load_test)
{
  WayElement e1(1 /* fake osm id */);

  using TBuffer = std::vector<uint8_t>;
  TBuffer buffer;
  MemWriter<TBuffer> w(buffer);

  e1.Write(w);

  MemReader r(buffer.data(), buffer.size());

  WayElement e2(1 /* fake osm id */);

  e2.Read(r);

  TEST_EQUAL(e2.m_nodes.size(), 0, ());
}

UNIT_TEST(Intermediate_Data_way_element_save_load_test)
{
  std::vector<uint64_t> testData = {0, 1, 2, 3, 0xFFFFFFFF, 0xFFFFFFFFFFFFFFFF};

  WayElement e1(1 /* fake osm id */);

  e1.m_nodes = testData;

  using TBuffer = std::vector<uint8_t>;
  TBuffer buffer;
  MemWriter<TBuffer> w(buffer);

  e1.Write(w);

  MemReader r(buffer.data(), buffer.size());

  WayElement e2(1 /* fake osm id */);

  e2.Read(r);

  TEST_EQUAL(e2.m_nodes, testData, ());
}

UNIT_TEST(Intermediate_Data_relation_element_save_load_test)
{
  std::vector<RelationElement::Member> testData = {{1, "inner"}, {2, "outer"}, {3, "unknown"}, {4, "inner role"}};

  RelationElement e1;

  e1.m_nodes = testData;
  e1.m_ways = testData;

  e1.m_tags.emplace("key1", "value1");
  e1.m_tags.emplace("key2", "value2");
  e1.m_tags.emplace("key3", "value3");
  e1.m_tags.emplace("key4", "value4");

  using TBuffer = std::vector<uint8_t>;
  TBuffer buffer;
  MemWriter<TBuffer> w(buffer);

  e1.Write(w);

  MemReader r(buffer.data(), buffer.size());

  RelationElement e2;

  e2.m_nodes.emplace_back(30, "000unknown");
  e2.m_nodes.emplace_back(40, "000inner role");
  e2.m_ways.emplace_back(10, "000inner");
  e2.m_ways.emplace_back(20, "000outer");
  e2.m_tags.emplace("key1old", "value1old");
  e2.m_tags.emplace("key2old", "value2old");

  e2.Read(r);

  TEST_EQUAL(e2.m_nodes, testData, ());
  TEST_EQUAL(e2.m_ways, testData, ());

  TEST_EQUAL(e2.m_tags.size(), 4, ());
  TEST_EQUAL(e2.m_tags["key1"], "value1", ());
  TEST_EQUAL(e2.m_tags["key2"], "value2", ());
  TEST_EQUAL(e2.m_tags["key3"], "value3", ());
  TEST_EQUAL(e2.m_tags["key4"], "value4", ());

  TEST_NOT_EQUAL(e2.m_tags["key1old"], "value1old", ());
  TEST_NOT_EQUAL(e2.m_tags["key2old"], "value2old", ());
}
}  // namespace intermediate_data_test
