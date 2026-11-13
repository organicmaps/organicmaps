#include "testing/testing.hpp"

#include "indexer/v2/ids_packet.hpp"

#include "coding/byte_stream.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace ids_packet_tests
{
using idx::v2::IdsPacket;
using Singleton = IdsPacket::Singleton;
using Ids = std::vector<uint32_t>;
using Bytes = std::vector<uint8_t>;

uint32_t constexpr kMaxId = std::numeric_limits<uint32_t>::max();

Ids Range(uint32_t first, uint32_t count, uint32_t step = 1)
{
  Ids ids;
  for (uint32_t i = 0; i < count; ++i)
    ids.push_back(first + i * step);
  return ids;
}

Bytes Write(Ids const & ids, uint64_t first, Singleton singleton = Singleton::Split)
{
  Bytes bytes;
  MemWriter writer(bytes);
  IdsPacket::Write(writer, ids, first, singleton);
  return bytes;
}

// A packet with the first id as is.
Bytes Write(Ids const & ids)
{
  return Write(ids, ids.front());
}

// Checks that the packet is read back and returns its size.
size_t Roundtrip(Ids const & ids)
{
  auto const bytes = Write(ids);
  MemReader reader(bytes.data(), bytes.size());
  ReaderSource source(reader);
  Ids decoded;
  uint32_t link = 0;
  TEST(IdsPacket::Read(source, Singleton::Split, link, [](uint64_t first) { return static_cast<uint32_t>(first); },
                       [&decoded](uint32_t id) { decoded.push_back(id); }),
       ());
  TEST_EQUAL(source.Size(), 0, ());
  TEST_EQUAL(decoded, ids, ());
  return bytes.size();
}

UNIT_TEST(IdsPacket_Layouts)
{
  // The control byte: the packet kind in three low bits, five inline bits above.
  TEST_EQUAL(Write({5}), (Bytes{(5 << 3) | 1, 0}), ());
  TEST_EQUAL(Write({1000}), (Bytes{(8 << 3) | 1, 31}), ());
  TEST_EQUAL(Write({1, 1000}), (Bytes{2, 1, 0xe7, 0x07}), ());
  TEST_EQUAL(Write({10, 11, 12, 13, 14, 15, 16, 17, 18, 19}), (Bytes{(1 << 3) | 4, 10, 10}), ());
  // Equal sizes keep the delta packet.
  TEST_EQUAL(Write({0, 1}), (Bytes{2, 0, 1}), ());
  // Every other id of two words: a bitmap from the first id.
  Bytes bitmap{(1 << 3) | 3, 3, 2};
  bitmap.insert(bitmap.end(), 16, 0x55);
  TEST_EQUAL(Write(Range(97, 64, 2)), bitmap, ());
}

UNIT_TEST(IdsPacket_Sizes)
{
  // Singletons: the low five bits are inline.
  for (uint32_t const id : {0U, 31U, 32U, 4095U, 4096U, kMaxId})
    TEST_EQUAL(Roundtrip({id}), id < 4096 ? 2 : (id == kMaxId ? 5 : 3), (id));

  // The count of a delta packet continues after the control byte from 33 ids.
  for (auto const & [count, countBytes] : {std::pair{2U, 0U}, {32U, 0U}, {33U, 1U}, {160U, 1U}, {161U, 2U}})
    TEST_EQUAL(Roundtrip(Range(1000, count, 1000)), 1 + countBytes + 2 * count, (count));

  // Runs: the count continues after the control byte from 31 runs; a run may end at the last id.
  TEST_EQUAL(Roundtrip(Range(0, 100000)), 5, ());
  TEST_EQUAL(Roundtrip(Range(kMaxId - 9, 10)), 7, ());
  for (uint32_t const runs : {2U, 30U, 31U, 32U, 100U})
  {
    Ids ids;
    for (uint32_t i = 0; i < runs; ++i)
      for (uint32_t j = 0; j < 20; ++j)
        ids.push_back(i * 100000 + j);
    TEST_EQUAL(Roundtrip(ids), 1 + (runs >= 31) + 2 + 4 * (runs - 1), (runs));
  }

  // Bitmaps: word boundaries, a base beyond the inline bits and the last id.
  TEST_EQUAL(Roundtrip(Range(0, 32, 2)), 1 + 1 + 1 + 8, ());
  TEST_EQUAL(Roundtrip(Range(1000000, 96, 2)), 1 + 3 + 1 + 24, ());
  TEST_EQUAL(Roundtrip(Range(kMaxId - 126, 64, 2)), 1 + 4 + 1 + 16, ());
  // A sparse tail after a dense head still needs every word, so deltas win.
  auto mixed = Range(0, 40, 2);
  mixed.push_back(1 << 20);
  TEST_EQUAL(Roundtrip(mixed), 1 + 1 + 40 + 3, ());
}

UNIT_TEST(IdsPacket_References)
{
  // The low five bits of a link are inline.
  for (uint32_t const link : {0U, 31U, 32U, 4095U, 4096U, (1U << 30) - 1})
  {
    Bytes bytes;
    MemWriter writer(bytes);
    IdsPacket::WriteReference(writer, link);
    TEST_EQUAL(bytes.size(), link < 4096 ? 2 : (link < (1U << 19) ? 3 : 5), (link));

    MemReader reader(bytes.data(), bytes.size());
    ReaderSource source(reader);
    uint32_t decoded = kMaxId;
    TEST(!IdsPacket::Read(source, Singleton::Split, decoded, [](uint64_t) { return 0U; },
                          [](uint32_t) { TEST(false, ("A reference has no ids")); }),
         ());
    TEST_EQUAL(decoded, link, ());
    TEST_EQUAL(source.Size(), 0, ());
  }
}

// The first id of a packet may be stored as any value, and a singleton in two ways.
UNIT_TEST(IdsPacket_FirstValue)
{
  TEST_EQUAL(Write({1000}, 5, Singleton::Count), (Bytes{(5 << 3) | 1}), ());
  TEST_EQUAL(Write({1000}, 5, Singleton::Split), (Bytes{(5 << 3) | 1, 0}), ());
  TEST_EQUAL(Write({1000}, 30, Singleton::Count), (Bytes{(30 << 3) | 1}), ());
  TEST_EQUAL(Write({1000}, 31, Singleton::Count), (Bytes{(31 << 3) | 1, 0}), ());
  TEST_EQUAL(Write({1000}, 1000, Singleton::Count), (Bytes{(31 << 3) | 1, 0xc9, 0x07}), ());
  // Only the first value of the other packets depends on the caller.
  TEST_EQUAL(Write({1000, 1001, 1005}, 3, Singleton::Count), (Bytes{(1 << 3) | 2, 3, 1, 4}), ());
  TEST_EQUAL(Write(Range(1000, 10), 2, Singleton::Count), (Bytes{(1 << 3) | 4, 2, 10}), ());

  std::vector<Ids> const lists = {
      {5},
      {1000},
      {kMaxId},
      {7, 8, 9},
      {1, 1000},
      Range(100, 200),
      Range(97, 64, 2),
      Range(1000, 40, 1000),
      {0, 1, 2, 10, 11, 12, 100, 101, 102, kMaxId - 1, kMaxId},
      Range(kMaxId - 126, 64, 2),
  };
  // The values of the first ids, up to the sum of two ids.
  uint64_t const firsts[] = {0, 1, 30, 31, 32, 4095, 4096, uint64_t{1} << 20, uint64_t{kMaxId} + kMaxId};

  for (auto const singleton : {Singleton::Split, Singleton::Count})
  {
    Bytes bytes;
    MemWriter writer(bytes);
    size_t count = 0;
    for (auto const & ids : lists)
      for (auto const first : firsts)
        IdsPacket::Write(writer, ids, first + count++, singleton);

    // The packets are read and skipped one after another.
    MemReader reader(bytes.data(), bytes.size());
    ReaderSource source(reader);
    ArrayByteSource skipped(bytes.data());
    count = 0;
    for (auto const & ids : lists)
    {
      for (auto const first : firsts)
      {
        Ids decoded;
        uint32_t link = 0;
        bool const isIds = IdsPacket::Read(source, singleton, link, [&](uint64_t value)
        {
          TEST_EQUAL(value, first + count, ());
          return ids.front();
        }, [&decoded](uint32_t id) { decoded.push_back(id); });
        TEST(isIds, ());
        TEST_EQUAL(decoded, ids, (first));
        TEST_EQUAL(IdsPacket::Skip(skipped, singleton), first + count, ());
        ++count;
      }
    }
    TEST_EQUAL(source.Size(), 0, ());
    TEST_EQUAL(skipped.PtrUint8(), bytes.data() + bytes.size(), ());
  }
}
}  // namespace ids_packet_tests
