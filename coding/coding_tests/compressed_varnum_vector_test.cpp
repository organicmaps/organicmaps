#include "../../testing/testing.hpp"

#include "../compressed_varnum_vector.hpp"
#include "../reader.hpp"
#include "../writer.hpp"

#include "../../base/pseudo_random.hpp"

namespace {
  uint64_t GetRand64()
  {
    static PseudoRNG32 g_rng;
    uint64_t result = g_rng.Generate();
    result ^= uint64_t(g_rng.Generate()) << 32;
    return result;
  }
}

struct NumsSource
{
  NumsSource(vector<uint64_t> const & v) : m_v(v) {}
  uint64_t operator()(uint64_t pos) { ASSERT_LESS(pos, m_v.size(), ()); return m_v[pos]; }
  
  vector<uint64_t> const & m_v;
};

UNIT_TEST(CompressedVarnumVector)
{
  uint32_t const NUMS_CNT = 5000;
  uint32_t const MAX_NUM_BYTESIZE = 5;
  vector<uint64_t> nums, sums(1, 0);
  uint64_t sum = 0;
  for (uint32_t i = 0; i < NUMS_CNT; ++i)
  {
    uint32_t byteSize = GetRand64() % MAX_NUM_BYTESIZE + 1;
    uint64_t num = GetRand64() & ((uint64_t(1) << (byteSize * 8)) - 1);
    nums.push_back(num);
    sum += num;
    sums.push_back(sum);
  }
  vector<uint8_t> encodedVector;
  MemWriter< vector<uint8_t> > encodedVectorWriter(encodedVector);
  BuildCompressedVarnumVector(encodedVectorWriter, NumsSource(nums), nums.size(), true);
  MemReader reader(encodedVector.data(), encodedVector.size());
  CompressedVarnumVectorReader comprNums(reader);
  // Find by index.
  for (uint32_t i = 0; i < nums.size(); ++i)
  {
    uint64_t sumBefore = 0;
    comprNums.FindByIndex(i, sumBefore);
    TEST_EQUAL(sumBefore, sums[i], ());
    uint64_t num = comprNums.Read();
    TEST_EQUAL(num, nums[i], ());
  }
  // Sequential read.
  uint64_t sumBefore = 0;
  comprNums.FindByIndex(200, sumBefore);
  for (uint32_t i = 200; i < 300; ++i)
  {
    uint64_t num = comprNums.Read();
    TEST_EQUAL(num, nums[i], ());
  }
  // Find by sum.
  for (uint32_t i = 1; i < nums.size() - 1; ++i)
  {
    // Find strict equal sum.
    {
      uint64_t sumIncl = 0, cntIncl = 0;
      uint64_t num = comprNums.FindBySum(sums[i], sumIncl, cntIncl);
      TEST_EQUAL(sumIncl, sums[i], ());
      TEST_EQUAL(cntIncl, i, ());
      TEST_EQUAL(num, nums[i - 1], ());
    }
    // Find by intermediate sum (not strictly equal).
    {
      uint64_t sumIncl = 0, cntIncl = 0;
      uint64_t num = comprNums.FindBySum(sums[i] + 1, sumIncl, cntIncl);
      TEST_EQUAL(sumIncl, sums[i + 1], ());
      TEST_EQUAL(cntIncl, i + 1, ());
      TEST_EQUAL(num, nums[i], ());
    }
  }
}
