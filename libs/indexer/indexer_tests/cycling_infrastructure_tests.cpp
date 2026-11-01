#include "testing/testing.hpp"

#include "indexer/cycling_infrastructure_section.hpp"

#include <limits>

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

UNIT_TEST(CyclingInfrastructure_RejectNonFiniteBitsWithFastMath)
{
  cycling::Sides sides{{cycling::Kind::Lane, cycling::Direction::Forward}, {}};
  sides.m_left.m_junctions = {{1, 2}};
  for (uint64_t bits : {uint64_t{0x7ff0000000000000}, uint64_t{0xfff0000000000000}, uint64_t{0x7ff8000000000000},
                        uint64_t{0x7ff0000000000001}, uint64_t{0xfff8000000000001}})
  {
    for (size_t coordinate : {size_t{0}, size_t{1}})
    {
      auto payload = cycling::Encode(sides);
      // Write the raw wire value, without asking the fast-math compiler to
      // materialize a non-finite floating-point value in ordinary arithmetic.
      auto const offset = 4 + coordinate * sizeof(uint64_t);
      for (size_t byte = 0; byte < sizeof(bits); ++byte)
        payload[offset + byte] = static_cast<char>((bits >> (8 * byte)) & 0xff);
      TEST(!cycling::Decode(payload), (bits, coordinate));
    }
  }
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
