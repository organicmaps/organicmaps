#include "testing/testing.hpp"

#include "coding/bit_streams.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "std/random.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"


UNIT_TEST(BitStreams_Smoke)
{
  uniform_int_distribution<uint8_t> randomBytesDistribution(0, 255);
  mt19937 rng(0);
  vector<pair<uint8_t, uint32_t>> nums;
  for (size_t i = 0; i < 100; ++i)
  {
    uint32_t numBits = randomBytesDistribution(rng) % 8;
    uint8_t num = randomBytesDistribution(rng) >> (CHAR_BIT - numBits);
    nums.push_back(make_pair(num, numBits));
  }
  for (size_t i = 0; i < 100; ++i)
  {
    uint32_t numBits = 8;
    uint8_t num = randomBytesDistribution(rng);
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
