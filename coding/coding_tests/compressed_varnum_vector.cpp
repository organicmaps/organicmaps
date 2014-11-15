#include <iostream>

#include "../compressed_varnum_vector.hpp"
#include "../reader.hpp"
#include "../writer.hpp"

#include "../../testing/testing.hpp"
#include "../../base/pseudo_random.hpp"

using std::vector;

namespace {
  uint64_t GetRand64() {
    static PseudoRNG32 g_rng;
    uint64_t result = g_rng.Generate();
    result ^= uint64_t(g_rng.Generate()) << 32;
    return result;
  }
}

struct NumsSource {
  NumsSource(vector<u64> const & v) : v_(v) {}
  u64 operator()(u64 pos) { ASSERT_LESS(pos, v_.size(), ()); return v_[pos]; }
  
  vector<u64> const & v_;
};

UNIT_TEST(CompressedVarnumVector) {
  u32 const c_nums_cnt = 5000;
  u32 const c_max_num_bytesize = 5;
  vector<u64> nums, sums(1, 0);
  u64 sum = 0;
  for (u32 i = 0; i < c_nums_cnt; ++i) {
    u32 byte_size = GetRand64() % c_max_num_bytesize + 1;
    u64 num = GetRand64() & ((u64(1) << (byte_size * 8)) - 1);
    nums.push_back(num);
    sum += num;
    sums.push_back(sum);
  }
  vector<u8> encoded_vector;
  MemWriter< vector<u8> > encoded_vector_writer(encoded_vector);
  BuildCompressedVarnumVector(encoded_vector_writer, NumsSource(nums), nums.size(), true);
  MemReader reader(encoded_vector.data(), encoded_vector.size());
  CompressedVarnumVectorReader compr_nums(reader);
  // Find by index.
  for (u32 i = 0; i < nums.size(); ++i) {
    u64 sum_before = 0;
    compr_nums.FindByIndex(i, sum_before);
    TEST_EQUAL(sum_before, sums[i], ());
    u64 num = compr_nums.Read();
    TEST_EQUAL(num, nums[i], ());
  }
  // Sequential read.
  u64 sum_before = 0;
  compr_nums.FindByIndex(200, sum_before);
  for (u32 i = 200; i < 300; ++i) {
    u64 num = compr_nums.Read();
    TEST_EQUAL(num, nums[i], ());
  }
  // Find by sum.
  for (u32 i = 1; i < nums.size() - 1; ++i) {
    // Find strict equal sum.
    {
      u64 sum_incl = 0, cnt_incl = 0;
      u64 num = compr_nums.FindBySum(sums[i], sum_incl, cnt_incl);
      TEST_EQUAL(sum_incl, sums[i], ());
      TEST_EQUAL(cnt_incl, i, ());
      TEST_EQUAL(num, nums[i - 1], ());
    }
    // Find by intermediate sum (not strictly equal).
    {
      u64 sum_incl = 0, cnt_incl = 0;
      u64 num = compr_nums.FindBySum(sums[i] + 1, sum_incl, cnt_incl);
      TEST_EQUAL(sum_incl, sums[i + 1], ());
      TEST_EQUAL(cnt_incl, i + 1, ());
      TEST_EQUAL(num, nums[i], ());
    }
  }
}
