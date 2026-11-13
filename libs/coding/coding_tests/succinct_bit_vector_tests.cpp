#include "testing/testing.hpp"

#include "3party/succinct/bit_vector.hpp"
#include "3party/succinct/broadword.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace
{
UNIT_TEST(SuccinctBitVectorBuilder_Reserve)
{
  for (uint64_t size : {0, 1, 63, 64, 65, 127, 128})
  {
    succinct::bit_vector_builder builder(size, true);
    builder.reserve(4096);
    // Writes must reach the reallocated storage; ASan also detects accesses to the freed buffer.
    builder.push_back(true);
    builder.append_bits(0b10, 2);

    succinct::bit_vector bits(&builder);
    TEST_EQUAL(bits.size(), size + 3, (size));
    for (uint64_t i = 0; i <= size; ++i)
      TEST(bits[i], (size, i));
    TEST(!bits[size + 1], (size));
    TEST(bits[size + 2], (size));
  }
}

UNIT_TEST(SuccinctBitVectorBuilder_AppendsAndSwap)
{
  succinct::bit_vector_builder builder;
  builder.zero_extend(0);
  builder.append_bits(0, 0);
  builder.push_back(true);
  builder.append_bits(0b10, 2);
  std::vector<bool> expected = {true, false, true};

  builder.zero_extend(62);
  expected.insert(expected.end(), 62, false);
  builder.one_extend(65);
  expected.insert(expected.end(), 65, true);

  succinct::bit_vector_builder suffix(3, true);
  builder.append(suffix);
  expected.insert(expected.end(), 3, true);

  builder.reverse();
  std::reverse(expected.begin(), expected.end());

  succinct::bit_vector_builder swapped;
  builder.swap(swapped);
  TEST_EQUAL(builder.size(), 0, ());
  builder.push_back(false);

  succinct::bit_vector bits(&swapped);
  TEST_EQUAL(bits.size(), expected.size(), ());
  for (uint64_t i = 0; i < bits.size(); ++i)
    TEST_EQUAL(bits[i], expected[i], (i));

  succinct::bit_vector other(&builder);
  TEST_EQUAL(other.size(), 1, ());
  TEST(!other[0], ());
}

// Check boundary and mixed words, including unsigned low-bit isolation at bit 63.
UNIT_TEST(SuccinctBroadword_LsbMsb)
{
  for (int i = 0; i < 64; ++i)
  {
    uint64_t const bit = uint64_t{1} << i;
    TEST_EQUAL(succinct::broadword::lsb(bit), i, ());
    TEST_EQUAL(succinct::broadword::msb(bit), i, ());
    // With all the bits above and below.
    TEST_EQUAL(succinct::broadword::lsb(~(bit - 1)), i, ());
    TEST_EQUAL(succinct::broadword::msb(bit | (bit - 1)), i, ());
  }

  unsigned long pos = 0;
  TEST(!succinct::broadword::lsb(0, pos), ());
  TEST(!succinct::broadword::msb(0, pos), ());
}
}  // namespace
