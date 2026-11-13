#include "testing/testing.hpp"

#include "coding/reader.hpp"
#include "coding/reader_wrapper.hpp"
#include "coding/succinct_mapper.hpp"
#include "coding/writer.hpp"

#include <cstdint>
#include <memory>
#include <random>
#include <utility>
#include <vector>

#include "3party/succinct/elias_fano.hpp"
#include "3party/succinct/mapper.hpp"
#include "3party/succinct/rs_bit_vector.hpp"

using namespace coding;

UNIT_TEST(ReverseMapper_Smoke)
{
  uint64_t data = 0x0123456789abcdef;
  uint64_t rdata = 0x0;
  TEST_EQUAL(8, ReverseMap(rdata, reinterpret_cast<uint8_t *>(&data), "uint64_t"), ());

  // Test that reversed uint64_t was read.
  TEST_EQUAL(0xefcdab8967452301, rdata, ());

  // Test that underlying buffer was modified.
  TEST_EQUAL(0xefcdab8967452301, data, ());
}

UNIT_TEST(Freeze_Smoke)
{
  std::vector<uint8_t> data;
  {
    MemWriter<decltype(data)> writer(data);
    uint64_t const data = 0x0123456789abcdef;
    Freeze(data, writer, "uint64_t");
  }
  TEST_EQUAL(8, data.size(), ());

  uint64_t value = 0x0;
  TEST_EQUAL(8, Map(value, reinterpret_cast<uint8_t const *>(data.data()), "uint64_t"), ());
  TEST_EQUAL(0x0123456789abcdef, value, ());
}

UNIT_TEST(ReverseFreeze_Smoke)
{
  std::vector<uint8_t> data;
  {
    MemWriter<decltype(data)> writer(data);
    uint64_t const data = 0x0123456789abcdef;
    ReverseFreeze(data, writer, "uint64_t");
  }

  TEST_EQUAL(8, data.size(), ());

  uint64_t value = 0x0;
  TEST_EQUAL(8, Map(value, reinterpret_cast<uint8_t const *>(data.data()), "uint64_t"), ());
  TEST_EQUAL(0xefcdab8967452301, value, ());
}

namespace
{
// A frozen rs_bit_vector answers through |reader| what the mapped one does.
template <class TReader>
void CheckBitVector(std::vector<uint8_t> const & data, TReader const & reader, std::vector<bool> const & bits)
{
  succinct::rs_bit_vector mapped;
  TEST_EQUAL(Map(mapped, data.data(), "rs_bit_vector"), data.size(), ());

  succinct::rs_bit_vector_t<succinct::mapper::dd_vector<uint64_t, TReader>> read;
  TEST_EQUAL(ReaderMap(read, reader, "rs_bit_vector"), data.size(), ());

  TEST_EQUAL(read.size(), bits.size(), ());
  TEST_EQUAL(read.num_ones(), mapped.num_ones(), ());
  for (uint64_t i = 0; i < bits.size(); ++i)
  {
    TEST_EQUAL(read[i], bits[i], (i));
    TEST_EQUAL(read.rank(i), mapped.rank(i), (i));
  }
  TEST_EQUAL(read.rank(bits.size()), mapped.num_ones(), ());
  for (uint64_t i = 0; i < mapped.num_ones(); ++i)
    TEST_EQUAL(read.select(i), mapped.select(i), (i));
  for (uint64_t i = 0; i < mapped.num_zeros(); ++i)
    TEST_EQUAL(read.select0(i), mapped.select0(i), (i));
}

template <class TReader>
void CheckEliasFano(std::vector<uint8_t> const & data, TReader const & reader, std::vector<uint64_t> const & values)
{
  succinct::elias_fano mapped;
  TEST_EQUAL(Map(mapped, data.data(), "elias_fano"), data.size(), ());

  succinct::elias_fano_t<succinct::mapper::dd_vector<uint64_t, TReader>> read;
  TEST_EQUAL(ReaderMap(read, reader, "elias_fano"), data.size(), ());

  TEST_EQUAL(read.size(), mapped.size(), ());
  TEST_EQUAL(read.num_ones(), values.size(), ());
  for (uint64_t i = 0; i < values.size(); ++i)
    TEST_EQUAL(read.select(i), values[i], (i));
  for (uint64_t i = 0; i + 1 < values.size(); ++i)
    TEST_EQUAL(read.select_range(i), std::make_pair(values[i], values[i + 1]), (i));
  // Random access too, not only the neighbours.
  std::mt19937 rng(8);
  for (int i = 0; i < 5000; ++i)
  {
    uint64_t const index = rng() % values.size();
    TEST_EQUAL(read.select(index), values[index], (index));
    uint64_t const pos = rng() % (mapped.size() + 1);
    TEST_EQUAL(read.rank(pos), mapped.rank(pos), (pos));
  }
}
}  // namespace

// The same frozen structure answers the same from memory and through a reader: of memory, where
// every element is read by itself, and of another kind, where the elements are read by windows.
UNIT_TEST(ReaderMap_SameAsMap)
{
  using succinct::mapper::is_memory_reader;
  static_assert(is_memory_reader<MemReader>);
  static_assert(is_memory_reader<MemReaderWithExceptions>);
  static_assert(is_memory_reader<SubReaderWrapper<MemReader const>>);
  static_assert(!is_memory_reader<ReaderPtr<Reader>>);
  static_assert(!is_memory_reader<SubReaderWrapper<Reader>>);
  static_assert(!is_memory_reader<SubReaderWrapper<ReaderPtr<Reader> const>>);

  std::mt19937 rng(7);
  {
    std::vector<bool> bits;
    for (size_t i = 0; i < 20000; ++i)
      bits.push_back(i < 3000 ? rng() % 2 == 0 : rng() % 50 == 0);

    std::vector<uint8_t> data;
    {
      succinct::rs_bit_vector built(bits, true /* with_select_hints */, true /* with_select0_hints */);
      MemWriter<decltype(data)> writer(data);
      Freeze(built, writer, "rs_bit_vector");
    }

    CheckBitVector(data, MemReader(data.data(), data.size()), bits);
    CheckBitVector(data, ReaderPtr<Reader>(std::make_unique<MemReader>(data.data(), data.size())), bits);
  }

  {
    // Dense and sparse parts, to have the blocks of both kinds in the select directories.
    std::vector<uint64_t> values;
    uint64_t value = 0;
    for (size_t i = 0; i < 20000; ++i)
    {
      value += (i / 2500 % 2 == 0) ? rng() % 3 : rng() % 100000;
      values.push_back(value);
    }

    std::vector<uint8_t> data;
    {
      succinct::elias_fano::elias_fano_builder builder(values.back(), values.size());
      for (auto const v : values)
        builder.push_back(v);
      succinct::elias_fano built(&builder, true /* with_rank_index */);
      MemWriter<decltype(data)> writer(data);
      Freeze(built, writer, "elias_fano");
    }

    CheckEliasFano(data, MemReader(data.data(), data.size()), values);
    CheckEliasFano(data, ReaderPtr<Reader>(std::make_unique<MemReader>(data.data(), data.size())), values);
  }
}
