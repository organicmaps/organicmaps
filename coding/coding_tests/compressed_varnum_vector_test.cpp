#include "../../testing/testing.hpp"

#include "../compressed_varnum_vector.hpp"
#include "../reader.hpp"
#include "../writer.hpp"

#include "../../base/pseudo_random.hpp"

namespace {
  u64 GetRand64()
  {
    static PseudoRNG32 g_rng;
    u64 result = g_rng.Generate();
    result ^= u64(g_rng.Generate()) << 32;
    return result;
  }
}

struct NumsSource
{
  NumsSource(vector<u64> const & v) : m_v(v) {}
  u64 operator()(u64 pos) { ASSERT_LESS(pos, m_v.size(), ()); return m_v[pos]; }
  
  vector<u64> const & m_v;
};

UNIT_TEST(CompressedVarnumVector)
{
  u32 const NUMS_CNT = 5000;
  u32 const MAX_NUM_BYTESIZE = 5;
  vector<u64> nums, sums(1, 0);
  u64 sum = 0;
  for (u32 i = 0; i < NUMS_CNT; ++i)
  {
    u32 byteSize = GetRand64() % MAX_NUM_BYTESIZE + 1;
    u64 num = GetRand64() & ((u64(1) << (byteSize * 8)) - 1);
    nums.push_back(num);
    sum += num;
    sums.push_back(sum);
  }
  vector<u8> encodedVector;
  MemWriter< vector<u8> > encodedVectorWriter(encodedVector);
  BuildCompressedVarnumVector(encodedVectorWriter, NumsSource(nums), nums.size(), true);
  MemReader reader(encodedVector.data(), encodedVector.size());
  CompressedVarnumVectorReader comprNums(reader);
  // Find by index.
  for (u32 i = 0; i < nums.size(); ++i)
  {
    u64 sumBefore = 0;
    comprNums.FindByIndex(i, sumBefore);
    TEST_EQUAL(sumBefore, sums[i], ());
    u64 num = comprNums.Read();
    TEST_EQUAL(num, nums[i], ());
  }
  // Sequential read.
  u64 sumBefore = 0;
  comprNums.FindByIndex(200, sumBefore);
  for (u32 i = 200; i < 300; ++i)
  {
    u64 num = comprNums.Read();
    TEST_EQUAL(num, nums[i], ());
  }
  // Find by sum.
  for (u32 i = 1; i < nums.size() - 1; ++i)
  {
    // Find strict equal sum.
    {
      u64 sumIncl = 0, cntIncl = 0;
      u64 num = comprNums.FindBySum(sums[i], sumIncl, cntIncl);
      TEST_EQUAL(sumIncl, sums[i], ());
      TEST_EQUAL(cntIncl, i, ());
      TEST_EQUAL(num, nums[i - 1], ());
    }
    // Find by intermediate sum (not strictly equal).
    {
      u64 sumIncl = 0, cntIncl = 0;
      u64 num = comprNums.FindBySum(sums[i] + 1, sumIncl, cntIncl);
      TEST_EQUAL(sumIncl, sums[i + 1], ());
      TEST_EQUAL(cntIncl, i + 1, ());
      TEST_EQUAL(num, nums[i], ());
    }
  }
}
