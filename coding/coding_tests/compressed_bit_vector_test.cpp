#include "../compressed_bit_vector.hpp"
#include "../reader.hpp"
#include "../writer.hpp"

#include "../../testing/testing.hpp"
#include "../../base/pseudo_random.hpp"

uint32_t const c_nums_count = 12345;

namespace {
  uint64_t GetRand64() {
    static PseudoRNG32 g_rng;
    uint64_t result = g_rng.Generate();
    result ^= uint64_t(g_rng.Generate()) << 32;
    return result;
  }
}

UNIT_TEST(CompressedBitVector_Sparse) {
  vector<uint32_t> pos_ones;
  uint32_t sum = 0;
  for (uint32_t i = 0; i < c_nums_count; ++i) {
    uint32_t byte_size = GetRand64() % 2 + 1;
    uint64_t num = GetRand64() & ((uint64_t(1) << (byte_size * 7)) - 1);
    if (num == 0) num = 1;
    sum += num;
    pos_ones.push_back(sum);
  }
  for (uint32_t j = 0; j < 5; ++j) {
    if (j == 1) pos_ones.insert(pos_ones.begin(), 1, 0);
    if (j == 2) pos_ones.clear();
    if (j == 3) pos_ones.push_back(1);
    if (j == 4) { pos_ones.clear(); pos_ones.push_back(10); }
    for (uint32_t ienc = 0; ienc < 4; ++ienc) {
      vector<uint8_t> serial_bit_vector;
      MemWriter< vector<uint8_t> > writer(serial_bit_vector);
      BuildCompressedBitVector(writer, pos_ones, ienc);
      MemReader reader(serial_bit_vector.data(), serial_bit_vector.size());
      vector<uint32_t> dec_pos_ones = DecodeCompressedBitVector(reader);
      TEST_EQUAL(pos_ones, dec_pos_ones, ());
    }
  }
}

UNIT_TEST(CompressedBitVector_Dense) {
  vector<uint32_t> pos_ones;
  uint32_t prev_pos = 0;
  uint32_t sum = 0;
  for (uint32_t i = 0; i < c_nums_count; ++i) {
    uint32_t zeroes_byte_size = GetRand64() % 2 + 1;
    uint64_t zeroes_range_size = (GetRand64() & ((uint64_t(1) << (zeroes_byte_size * 7)) - 1)) + 1;
    sum += zeroes_range_size;
    uint32_t ones_byte_size = GetRand64() % 1 + 1;
    uint64_t ones_range_size = (GetRand64() & ((uint64_t(1) << (ones_byte_size * 7)) - 1)) + 1;
    for (uint32_t j = 0; j < ones_range_size; ++j) pos_ones.push_back(sum + j);
    sum += ones_range_size;
  }
  for (uint32_t j = 0; j < 5; ++j) {
    if (j == 1) pos_ones.insert(pos_ones.begin(), 1, 0);
    if (j == 2) pos_ones.clear();
    if (j == 3) pos_ones.push_back(1);
    if (j == 4) { pos_ones.clear(); pos_ones.push_back(10); }
    for (uint32_t ienc = 0; ienc < 4; ++ienc) {
      vector<uint8_t> serial_bit_vector;
      MemWriter< vector<uint8_t> > writer(serial_bit_vector);
      BuildCompressedBitVector(writer, pos_ones, ienc);
      MemReader reader(serial_bit_vector.data(), serial_bit_vector.size());
      vector<uint32_t> dec_pos_ones = DecodeCompressedBitVector(reader);
      TEST_EQUAL(pos_ones, dec_pos_ones, ());
    }
  }
}

UNIT_TEST(BitVectors_And) {
  vector<bool> v1(c_nums_count * 2, false), v2(c_nums_count * 2, false);
  for (uint32_t i = 0; i < c_nums_count; ++i) {
    v1[GetRand64() % v1.size()] = true;
    v2[GetRand64() % v2.size()] = true;
  }
  vector<uint32_t> pos_ones1, pos_ones2, and_pos;
  for (uint32_t i = 0; i < v1.size(); ++i) {
    if (v1[i]) pos_ones1.push_back(i);
    if (v2[i]) pos_ones2.push_back(i);
    if (v1[i] && v2[i]) and_pos.push_back(i);
  }
  vector<uint32_t> actual_and_pos = BitVectorsAnd(pos_ones1.begin(), pos_ones1.end(), pos_ones2.begin(), pos_ones2.end());
  TEST_EQUAL(and_pos, actual_and_pos, ());
}

UNIT_TEST(BitVectors_Or) {
  vector<bool> v1(c_nums_count * 2, false), v2(c_nums_count * 2, false);
  for (uint32_t i = 0; i < c_nums_count; ++i) {
    v1[GetRand64() % v1.size()] = true;
    v2[GetRand64() % v2.size()] = true;
  }
  vector<uint32_t> pos_ones1, pos_ones2, or_pos;
  for (uint32_t i = 0; i < v1.size(); ++i) {
    if (v1[i]) pos_ones1.push_back(i);
    if (v2[i]) pos_ones2.push_back(i);
    if (v1[i] || v2[i]) or_pos.push_back(i);
  }
  vector<uint32_t> actual_or_pos = BitVectorsOr(pos_ones1.begin(), pos_ones1.end(), pos_ones2.begin(), pos_ones2.end());
  TEST_EQUAL(or_pos, actual_or_pos, ());
}

UNIT_TEST(BitVectors_SubAnd) {
  vector<bool> v1(c_nums_count * 2, false);
  uint64_t num_v1_ones = 0;
  for (uint32_t i = 0; i < v1.size(); ++i) v1[i] = (GetRand64() % 2) == 0;
  vector<uint32_t> pos_ones1;
  for (uint32_t i = 0; i < v1.size(); ++i) if (v1[i]) pos_ones1.push_back(i);
  vector<bool> v2(pos_ones1.size(), false);
  for (uint32_t i = 0; i < v2.size(); ++i) v2[i] = (GetRand64() % 2) == 0;
  vector<uint32_t> pos_ones2, suband_pos;
  for (uint32_t i = 0; i < v2.size(); ++i) if (v2[i]) pos_ones2.push_back(i);
  for (uint32_t i = 0, j = 0; i < v1.size(); ++i) {
    if (v1[i]) {
      if (v2[j]) suband_pos.push_back(i);
      ++j;
    }
  }
  vector<uint32_t> actual_suband_pos = BitVectorsSubAnd(pos_ones1.begin(), pos_ones1.end(), pos_ones2.begin(), pos_ones2.end());
  TEST_EQUAL(suband_pos, actual_suband_pos, ());
}
