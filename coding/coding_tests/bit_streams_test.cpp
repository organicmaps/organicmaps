#include "testing/testing.hpp"

#include "coding/bit_streams.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "std/random.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"


UNIT_TEST(BitStream_ReadWrite)
{
  mt19937 rng(0);
  uint32_t const NUMS_CNT = 1000;
  vector< pair<uint64_t, uint32_t> > nums;
  for (uint32_t i = 0; i < NUMS_CNT; ++i)
  {
    uint32_t numBits = rng() % 65;
    uint64_t num = rng() & ((uint64_t(1) << numBits) - 1);
    nums.push_back(make_pair(num, numBits));
  }
  
  vector<uint8_t> encodedBits;
  {
    MemWriter< vector<uint8_t> > encodedBitsWriter(encodedBits);
    BitSink bitsSink(encodedBitsWriter);
    for (uint32_t i = 0; i < nums.size(); ++i) bitsSink.Write(nums[i].first, nums[i].second);
  }
  MemReader encodedBitsReader(encodedBits.data(), encodedBits.size());
  BitSource bitsSource(encodedBitsReader);
  for (uint32_t i = 0; i < nums.size(); ++i)
  {
    uint64_t num = bitsSource.Read(nums[i].second);
    TEST_EQUAL(num, nums[i].first, ());
  }
}

