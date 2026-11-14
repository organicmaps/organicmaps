#include "testing/testing.hpp"

#include "indexer/indexer_tests/counting_reader.hpp"
#include "indexer/v2/succinct_wrapper.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/reader.hpp"
#include "coding/reader_wrapper.hpp"
#include "coding/writer.hpp"

#include "platform/platform.hpp"

#include "base/scope_guard.hpp"

#include <cstdint>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace succinct_wrapper_tests
{
using namespace idx::v2;
using indexer_tests::CountingReader;

// The part of a reader after the prefix: a structure after other data.
template <class TReader>
SubReaderWrapper<TReader const> GetPart(TReader const & reader, size_t prefix)
{
  return SubReaderWrapper<TReader const>(&reader).SubReader(prefix, reader.Size() - prefix);
}

template <class TBuilder>
std::vector<uint8_t> Flush(TBuilder & builder, size_t prefix)
{
  std::vector<uint8_t> bytes(prefix, 0xFF);
  MemWriter writer(bytes);
  writer.Seek(prefix);
  builder.Flush(writer);
  return bytes;
}

template <class TReader>
void CheckBits(TReader const & reader, std::vector<bool> const & bits)
{
  RsBitsVector<TReader> const vector(reader);
  TEST_EQUAL(vector.size(), bits.size(), ());
  uint64_t ones = 0;
  for (size_t i = 0; i < bits.size(); ++i)
  {
    TEST_EQUAL(vector[i], bits[i], (i));
    TEST_EQUAL(vector.rank(i), ones, (i));
    if (bits[i])
      TEST_EQUAL(vector.select(ones++), i, ());
  }
  TEST_EQUAL(vector.rank(bits.size()), ones, ());
  TEST_EQUAL(vector.num_ones(), ones, ());
}

template <class TReader>
void CheckNumbers(TReader const & reader, std::vector<uint64_t> const & numbers)
{
  EfBitsVector<TReader> const vector(reader);
  TEST_EQUAL(vector.num_ones(), numbers.size(), ());
  for (size_t i = 0; i < numbers.size(); ++i)
    TEST_EQUAL(vector.select(i), numbers[i], (i));
  for (size_t i = 0; i + 1 < numbers.size(); ++i)
    TEST_EQUAL(vector.select_range(i), std::make_pair(numbers[i], numbers[i + 1]), (i));
}

std::vector<bool> MakeBits(std::mt19937 & rng, size_t count, RsBitsVectorBuilder & builder)
{
  std::vector<bool> bits;
  for (size_t i = 0; i < count; ++i)
  {
    bits.push_back(rng() % 7 == 0);
    builder.PushBack(bits.back());
  }
  return bits;
}

std::vector<uint64_t> MakeNumbers(std::mt19937 & rng, size_t count, EfBitsVectorBuilder & builder)
{
  std::vector<uint64_t> numbers;
  for (uint64_t number = 0; numbers.size() < count; number += rng() % 1000)
  {
    numbers.push_back(number);
    builder.PushBack(number);
  }
  return numbers;
}

// The vectors work with any reader: of memory, of a part of other data, of a file.
UNIT_TEST(SuccinctWrapper_Readers)
{
  std::mt19937 rng(3);
  RsBitsVectorBuilder bitsBuilder(true /* selectHints */);
  auto const bits = MakeBits(rng, 5000, bitsBuilder);
  EfBitsVectorBuilder numbersBuilder;
  auto const numbers = MakeNumbers(rng, 3000, numbersBuilder);

  // A structure may start anywhere in the reader.
  for (size_t const prefix : {0, 8, 3})
  {
    auto const bitsBytes = Flush(bitsBuilder, prefix);
    auto const numbersBytes = Flush(numbersBuilder, prefix);

    MemReader const bitsReader(bitsBytes.data(), bitsBytes.size());
    CheckBits(GetPart(bitsReader, prefix), bits);
    MemReader const numbersReader(numbersBytes.data(), numbersBytes.size());
    CheckNumbers(GetPart(numbersReader, prefix), numbers);
  }

  std::string const path = GetPlatform().TmpPathForFile("succinct_wrapper_test", ".bits");
  SCOPE_GUARD(removeFile, [&path] { FileWriter::DeleteFileX(path); });
  uint64_t numbersPos = 0;
  {
    FileWriter writer(path);
    bitsBuilder.Flush(writer);
    numbersPos = writer.Pos();
    numbersBuilder.Flush(writer);
  }
  FileReader const reader(path);
  CheckBits(reader.SubReader(0, numbersPos), bits);
  CheckNumbers(reader.SubReader(numbersPos, reader.Size() - numbersPos), numbers);
}

// Nothing is loaded: a vector is opened by a few reads. With a reader of memory an operation reads
// the words that it needs one by one.
UNIT_TEST(SuccinctWrapper_ReadsOnDemand)
{
  using Reader = CountingReader<true /* IsMemoryReader */>;

  std::mt19937 rng(4);
  {
    RsBitsVectorBuilder builder;
    auto const bits = MakeBits(rng, 200000, builder);
    auto const bytes = Flush(builder, 0 /* prefix */);
    TEST_GREATER(bytes.size(), 30000, ());

    Reader const reader(bytes);
    RsBitsVector<Reader> const vector(reader);
    TEST_EQUAL(vector.size(), bits.size(), ());
    TEST_LESS(reader.GetReadBytes(), 100, ());

    reader.ResetCounters();
    TEST_EQUAL(vector[123456], bits[123456], ());
    TEST_EQUAL(reader.GetReads(), 1, ());
    TEST_EQUAL(reader.GetReadBytes(), 8, ());

    // The rank of a block, the ranks of its words, the word.
    reader.ResetCounters();
    vector.rank(123457);
    TEST_EQUAL(reader.GetReads(), 3, ());
    TEST_EQUAL(reader.GetReadBytes(), 24, ());
  }
  {
    EfBitsVectorBuilder builder;
    auto const numbers = MakeNumbers(rng, 100000, builder);
    auto const bytes = Flush(builder, 0 /* prefix */);
    TEST_GREATER(bytes.size(), 100000, ());

    Reader const reader(bytes);
    EfBitsVector<Reader> const vector(reader);
    TEST_LESS(reader.GetReadBytes(), 200, ());

    reader.ResetCounters();
    TEST_EQUAL(vector.select(54321), numbers[54321], ());
    TEST_LESS_OR_EQUAL(reader.GetReads(), 8, ());
    TEST_LESS_OR_EQUAL(reader.GetReadBytes(), 64, ());
  }
}

// With another reader, like the one of a file, a call costs more than its bytes: a vector reads
// a window of words at once, and the operations nearby do not read at all.
UNIT_TEST(SuccinctWrapper_ReadsByWindows)
{
  using Reader = CountingReader<false /* IsMemoryReader */>;
  uint64_t constexpr kWindowBytes = succinct::mapper::dd_vector<uint64_t, Reader>::window_size * sizeof(uint64_t);

  std::mt19937 rng(4);
  {
    RsBitsVectorBuilder builder;
    auto const bits = MakeBits(rng, 200000, builder);
    auto const bytes = Flush(builder, 0 /* prefix */);

    Reader const reader(bytes);
    RsBitsVector<Reader> const vector(reader);
    TEST_LESS(reader.GetReadBytes(), 100, ());

    reader.ResetCounters();
    TEST_EQUAL(vector[123456], bits[123456], ());
    TEST_EQUAL(reader.GetReads(), 1, ());
    TEST_EQUAL(reader.GetReadBytes(), kWindowBytes, ());

    // The window of the ranks is read, the word is in its window already.
    reader.ResetCounters();
    vector.rank(123457);
    TEST_EQUAL(reader.GetReads(), 1, ());
    TEST_EQUAL(reader.GetReadBytes(), kWindowBytes, ());

    reader.ResetCounters();
    uint64_t ones = vector.rank(123400);
    for (uint64_t i = 123400; i < 123900; ++i)
    {
      TEST_EQUAL(vector.rank(i), ones, (i));
      ones += vector[i] ? 1 : 0;
    }
    TEST_EQUAL(reader.GetReads(), 0, ());

    CheckBits(reader, bits);
  }
  {
    EfBitsVectorBuilder builder;
    auto const numbers = MakeNumbers(rng, 100000, builder);
    auto const bytes = Flush(builder, 0 /* prefix */);

    Reader const reader(bytes);
    EfBitsVector<Reader> const vector(reader);
    TEST_LESS(reader.GetReadBytes(), 200, ());

    // The select directory of two levels, the high bits, the low bits.
    reader.ResetCounters();
    TEST_EQUAL(vector.select(54321), numbers[54321], ());
    TEST_LESS_OR_EQUAL(reader.GetReads(), 5, ());

    reader.ResetCounters();
    TEST_EQUAL(vector.select(54322), numbers[54322], ());
    TEST_EQUAL(vector.select(54323), numbers[54323], ());
    TEST_EQUAL(reader.GetReads(), 0, ());

    CheckNumbers(reader, numbers);
  }
}
}  // namespace succinct_wrapper_tests
