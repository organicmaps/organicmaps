// Author: Artyom.
// A module for storing arbitrary variable-bitsize numbers in a compressed form so that later
// you can access any number searching it by index or sum of numbers preceeding and including searched number.

#pragma once

#include "../std/functional.hpp"
#include "../std/stdint.hpp"
#include "../std/vector.hpp"

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

// Forward declarations.
class Reader;
class Writer;

// Number of nums in a chunk per one table entry.
u64 const c_num_elem_per_table_entry = 1024;

// A source of nums.
typedef std::function<u64 (u64 pos)> NumsSourceFuncT;
// Builds CompressedVarnumVector based on source of numbers.
// If support_sums is true then sums are included in the table otherwise sums are not computed.
void BuildCompressedVarnumVector(Writer & writer, NumsSourceFuncT nums_source, u64 nums_cnt, bool support_sums);

// Reader of CompressedVarnumVector.
class CompressedVarnumVectorReader {
public:
  // Bytes are read from Reader on the flight while decoding.
  CompressedVarnumVectorReader(Reader & reader);
  ~CompressedVarnumVectorReader();

  // Set current number decoding context to number at given index.
  // sum_before will contain total sum of numbers before indexed number, computed only if sums are supported.
  void FindByIndex(u64 index, u64 & sum_before);
  // Works only if sums are supported. Finds ith number by total sum of numbers in the range [0, i], i.e.
  // finds such first number that sum of all number before and including it are equal or greater to sum.
  // sum_incl will contain the actual sum including found number, cnt_incl contains count of numbers including
  // found one. Function returns found number.
  u64 FindBySum(u64 sum, u64 & sum_incl, u64 & cnt_incl);
  // After setting position by FindByIndex and FindBySum functions Read() function will sequentially read
  // next number. It is only allowed to read numbers in same chunk as the first number found (one chunk is
  // created for one table entry).
  u64 Read();
private:
  void SetDecodeContext(u64 table_entry_index);
private:
  Reader & reader_;
  u64 nums_cnt_;
  u64 num_elem_per_table_entry_;
  bool support_sums_;
  u64 nums_encoded_offset_;
  std::vector<u32> distr_table_;
  std::vector<u64> table_pos_;
  std::vector<u64> table_sum_;
  // Decode context.
  struct DecodeContext;
  DecodeContext * decode_ctx_;
};
