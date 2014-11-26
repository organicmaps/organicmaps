#include "../../testing/testing.hpp"

#include "../bit_streams.hpp"
#include "../reader.hpp"
#include "../writer.hpp"

#include "../../base/pseudo_random.hpp"
#include "../../std/utility.hpp"
#include "../../std/vector.hpp"

namespace
{
  uint64_t GetRand64()
  {
    static PseudoRNG32 g_rng;
    uint64_t result = g_rng.Generate();
    result ^= uint64_t(g_rng.Generate()) << 32;
    return result;
  }
}

UNIT_TEST(BitStream_ReadWrite)
{
  uint32_t const NUMS_CNT = 1000;
  vector< pair<uint64_t, uint32_t> > nums;
  for (uint32_t i = 0; i < NUMS_CNT; ++i)
  {
    uint32_t numBits = GetRand64() % 57;
    uint64_t num = GetRand64() & (uint64_t(-1) >> (64 - numBits));
    // Right bit shift by 64 doesn't always work correctly,
    // this is a workaround.
    if (numBits == 0) num = 0;
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

