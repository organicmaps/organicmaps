#include "testing/testing.hpp"

#include "indexer/cycling_infrastructure_section.hpp"

#include <limits>
#include <map>

UNIT_TEST(CyclingInfrastructure_SideDirectionRoundTripAndReverse)
{
  cycling::Sides sides{{cycling::Kind::Track, cycling::Direction::Backward},
                       {cycling::Kind::Lane, cycling::Direction::Both}};
  sides.m_left.m_junctions = {{-123.12, 56.7}};
  auto restored = cycling::Decode(cycling::Encode(sides));
  TEST(restored, ());
  TEST_EQUAL(restored->m_left.m_junctions, sides.m_left.m_junctions, ());
  restored->Reverse();
  TEST_EQUAL(static_cast<char>(restored->m_right.m_kind), 't', ());
  TEST_EQUAL(static_cast<char>(restored->m_right.m_direction), 'f', ());
  TEST_EQUAL(static_cast<char>(restored->m_left.m_direction), 'b', ());
  TEST_EQUAL(restored->m_right.m_junctions, sides.m_left.m_junctions, ());
}

UNIT_TEST(CyclingInfrastructure_RejectMalformedPayloads)
{
  cycling::Sides sides{{cycling::Kind::Lane, cycling::Direction::Forward}, {}};
  sides.m_left.m_junctions = {{1, 2}};
  auto payload = cycling::Encode(sides);
  for (size_t size = 0; size < payload.size(); ++size)
    TEST(!cycling::Decode(std::string_view(payload).substr(0, size)), (size));
  TEST(!cycling::Decode(payload + '\0'), ());
  auto unknown = payload;
  unknown[0] = 127;
  TEST(!cycling::Decode(unknown), ());
  unknown = payload;
  unknown[1] = 127;
  TEST(!cycling::Decode(unknown), ());
  unknown = payload;
  unknown[3] = 65;
  TEST(!cycling::Decode(unknown), ());
  sides.m_left.m_junctions[0].x = std::numeric_limits<double>::infinity();
  TEST(!cycling::Decode(cycling::Encode(sides)), ());
}

UNIT_TEST(CyclingInfrastructure_ExplicitSidesAndSeparatelyMappedPaths)
{
  std::map<std::string, std::string> tags{
      {"cycleway:both", "lane"}, {"cycleway:left", "separate"}, {"cycleway:right:oneway", "-1"}};
  auto read = [&](std::string const & key)
  {
    auto it = tags.find(key);
    return it == tags.end() ? "" : it->second;
  };
  auto sides = cycling::ReadSides(read, false);
  TEST(sides, ());
  TEST_EQUAL(static_cast<char>(sides->m_left.m_kind), '0', ());
  TEST_EQUAL(static_cast<char>(sides->m_right.m_kind), 'l', ());
  TEST_EQUAL(static_cast<char>(sides->m_right.m_direction), 'r', ());
  tags = {{"cycleway", "lane"}};
  TEST(!cycling::ReadSides(read, false), ());
}

UNIT_TEST(CyclingInfrastructure_SparseSectionRoundTrip)
{
  auto const payload = cycling::Encode({{cycling::Kind::Lane, cycling::Direction::Forward}, {}});
  std::vector<uint8_t> data;
  PushBackByteSink sink(data);
  cycling::WriteSection(sink, {{2, payload}, {99, payload}});
  MemReader reader(data.data(), data.size());
  TEST_EQUAL(cycling::ReadSection(reader, 2), payload, ());
  TEST_EQUAL(cycling::ReadSection(reader, 99), payload, ());
  for (uint32_t id : {0, 1, 3, 98, 100})
    TEST(cycling::ReadSection(reader, id).empty(), (id));
}

UNIT_TEST(CyclingInfrastructure_TruncatedAndUnknownSectionsAreIgnored)
{
  auto const payload = cycling::Encode({{cycling::Kind::Track, cycling::Direction::Both}, {}});
  std::vector<uint8_t> data;
  PushBackByteSink sink(data);
  cycling::WriteSection(sink, {{2, payload}});
  for (size_t size = 0; size < data.size(); ++size)
  {
    MemReaderWithExceptions reader(data.data(), size);
    TEST(cycling::ReadSection(reader, 2).empty(), (size));
  }
  data[0] = 127;
  MemReader reader(data.data(), data.size());
  TEST(cycling::ReadSection(reader, 2).empty(), ());
}
