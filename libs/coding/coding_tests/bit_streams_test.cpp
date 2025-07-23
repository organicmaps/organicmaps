#include "testing/testing.hpp"

#include "coding/bit_streams.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/bits.hpp"

#include <cstddef>
#include <cstdint>
#include <random>
#include <utility>
#include <vector>

using namespace std;

namespace
{
UNIT_TEST(BitStreams_Smoke)
{
  uniform_int_distribution<uint32_t> randomBytesDistribution(0, 255);
  mt19937 rng(0);
  vector<pair<uint8_t, uint32_t>> nums;
  for (size_t i = 0; i < 100; ++i)
  {
    uint32_t numBits = randomBytesDistribution(rng) % 8;
    uint8_t num = static_cast<uint8_t>(randomBytesDistribution(rng) >> (CHAR_BIT - numBits));
    nums.push_back(make_pair(num, numBits));
  }
  for (size_t i = 0; i < 100; ++i)
  {
    uint32_t numBits = 8;
    uint8_t num = static_cast<uint8_t>(randomBytesDistribution(rng));
    nums.push_back(make_pair(num, numBits));
  }

  vector<uint8_t> encodedBits;
  {
    MemWriter<vector<uint8_t>> encodedBitsWriter(encodedBits);
    BitWriter<MemWriter<vector<uint8_t>>> bitSink(encodedBitsWriter);
    for (size_t i = 0; i < nums.size(); ++i)
      bitSink.Write(nums[i].first, nums[i].second);
  }

  MemReader encodedBitsReader(encodedBits.data(), encodedBits.size());
  ReaderSource<MemReader> src(encodedBitsReader);
  BitReader<ReaderSource<MemReader>> bitsSource(src);
  for (size_t i = 0; i < nums.size(); ++i)
  {
    uint8_t num = bitsSource.Read(nums[i].second);
    TEST_EQUAL(num, nums[i].first, (i));
  }
}

UNIT_TEST(BitStreams_T1)
{
  using TBuffer = vector<uint8_t>;
  using TWriter = MemWriter<TBuffer>;

  TBuffer buf;
  {
    TWriter w(buf);
    BitWriter<TWriter> bits(w);

    bits.Write(0, 3);
    bits.Write(3, 3);
    bits.Write(6, 3);
  }

  TEST_EQUAL(buf.size(), 2, ());
}

UNIT_TEST(BitStreams_Large)
{
  using TBuffer = vector<uint8_t>;
  using TWriter = MemWriter<TBuffer>;

  uint64_t const kMask = 0x0123456789abcdef;

  TBuffer buf;
  {
    TWriter w(buf);
    BitWriter<TWriter> bits(w);

    for (int i = 0; i <= 64; ++i)
      if (i <= 32)
        bits.WriteAtMost32Bits(static_cast<uint32_t>(kMask), i);
      else
        bits.WriteAtMost64Bits(kMask, i);
  }

  {
    MemReader r(buf.data(), buf.size());
    ReaderSource<MemReader> src(r);
    BitReader<ReaderSource<MemReader>> bits(src);
    for (int i = 0; i <= 64; ++i)
    {
      uint64_t const mask = bits::GetFullMask(i);
      uint64_t const value = i <= 32 ? bits.ReadAtMost32Bits(i) : bits.ReadAtMost64Bits(i);
      TEST_EQUAL(value, kMask & mask, (i));
    }
  }
}
}  // namespace
