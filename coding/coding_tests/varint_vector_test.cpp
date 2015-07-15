#include "testing/testing.hpp"

/*
#include "coding/reader.hpp"
#include "coding/varint_vector.hpp"
#include "coding/writer.hpp"

#include "std/random.hpp"

using namespace varint;


UNIT_TEST(VarintVector_Use)
{
  vector<uint8_t> buffer;
  MemWriter<vector<uint8_t>> writer(buffer);

  vector<uint64_t> g_nums;
  vector<uint64_t> g_nums_sums;

  uint32_t const c_nums_count = 12345;
  uint32_t const c_index_tests_count = 50000;
  uint32_t const c_sum_tests_count = 20000;

  mt19937 rng(0);

  // Generate vector.
  {
    uint64_t sum = 0;
    VectorBuilder builder;
    for (uint32_t i = 0; i < c_nums_count; ++i)
    {
      g_nums_sums.push_back(sum);
      uint8_t const byte_size = rng() % 6 + 1;
      uint64_t const num = rng() & ((uint64_t(1) << (byte_size * 7)) - 1);

      g_nums.push_back(num);
      builder.AddNum(num);
      sum += num;
    }

    TEST_EQUAL(g_nums.size(), c_nums_count, ());
    TEST_EQUAL(g_nums_sums.size(), c_nums_count, ());

    builder.Finalize(&writer);
  }

  MemReader reader(buffer.data(), buffer.size());

  // Test sequential access by index.
  {
    Vector v(&reader);
    for (uint32_t i = 0; i < c_nums_count; ++i)
    {
      uint32_t serial_pos = 0;
      uint64_t sum_before = 0;
      v.FindByIndex(i, serial_pos, sum_before);

      TEST_EQUAL(sum_before, g_nums_sums[i], ());
      uint64_t num = 0;
      v.Read(serial_pos, num);

      TEST_EQUAL(g_nums[i], num, ());
    }
  }

  // Test random access by index.
  {
    Vector v(&reader);
    for (uint32_t i = 0; i < c_index_tests_count; ++i)
    {
      uint64_t const index = rng() % g_nums.size();

      uint32_t serial_pos = 0;
      uint64_t sum_before = 0;
      v.FindByIndex(index, serial_pos, sum_before);

      TEST_EQUAL(sum_before, g_nums_sums[index], ());

      uint64_t num = 0;
      v.Read(serial_pos, num);
      TEST_EQUAL(g_nums[index], num, ());
    }
  }

  // Test sequential access by precise sum.
  {
    Vector v(&reader);
    for (uint32_t i = 0; i < c_nums_count-1; ++i)
    {
      if (g_nums_sums[i] == g_nums_sums[i + 1])
        continue;

      uint64_t const sum = g_nums_sums[i];

      uint32_t serial_pos = 0;
      uint64_t sum_before = 0;
      uint64_t count_before = 0;
      v.FindBySum(sum, serial_pos, sum_before, count_before);

      TEST_EQUAL(count_before, i, ());
      TEST_EQUAL(sum, sum_before, ());

      uint64_t num = 0;
      v.Read(serial_pos, num);
      TEST_EQUAL(g_nums[i], num, ());
    }
  }

  // Test random access by precise sum.
  {
    Vector v(&reader);
    for (uint32_t i = 0; i < c_sum_tests_count; ++i)
    {
      uint64_t index = rng() % (g_nums_sums.size() - 2);
      while (g_nums_sums[index] == g_nums_sums[index + 1])
      {
        ++index;
        TEST_LESS(index+1, g_nums.size(), ());
      }

      uint64_t const sum = g_nums_sums[index];

      uint32_t serial_pos = 0;
      uint64_t sum_before = 0;
      uint64_t count_before = 0;
      v.FindBySum(sum, serial_pos, sum_before, count_before);

      TEST_EQUAL(count_before, index, ());
      TEST_EQUAL(sum, sum_before, ());

      uint64_t num = 0;
      v.Read(serial_pos, num);
      TEST_EQUAL(g_nums[index], num, ());
    }
  }

  // Test random access by intermediate sum.
  {
    Vector v(&reader);
    for (uint32_t i = 0; i < c_sum_tests_count; ++i)
    {
      uint64_t index = rng() % (g_nums_sums.size() - 2);
      while (g_nums_sums[index] + 1 >= g_nums_sums[index + 1])
      {
        ++index;
        TEST_LESS(index+1, g_nums_sums.size(), ());
      }

      uint64_t const sum = (g_nums_sums[index] + g_nums_sums[index + 1]) / 2;

      uint32_t serial_pos = 0;
      uint64_t sum_before = 0;
      uint64_t count_before = 0;
      v.FindBySum(sum, serial_pos, sum_before, count_before);

      TEST_EQUAL(count_before, index, ());
      TEST_GREATER(sum, sum_before, ());
      TEST_LESS(sum, g_nums_sums[index + 1], ());

      uint64_t num = 0;
      v.Read(serial_pos, num);
      TEST_EQUAL(g_nums[index], num, ());
    }
  }
}
*/
