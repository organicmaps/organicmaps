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
u64 const NUM_ELEM_PER_TABLE_ENTRY = 1024;

// A source of nums.
typedef std::function<u64 (u64 pos)> NumsSourceFuncT;
// Builds CompressedVarnumVector based on source of numbers.
// If supportSums is true then sums are included in the table otherwise sums are not computed.
void BuildCompressedVarnumVector(Writer & writer, NumsSourceFuncT numsSource, u64 numsCnt, bool supportSums);

// Reader of CompressedVarnumVector.
class CompressedVarnumVectorReader
{
public:
  // Bytes are read from Reader on the flight while decoding.
  CompressedVarnumVectorReader(Reader & reader);
  ~CompressedVarnumVectorReader();

  // Set current number decoding context to number at given index.
  // sumBefore will contain total sum of numbers before indexed number, computed only if sums are supported.
  void FindByIndex(u64 index, u64 & sumBefore);
  // Works only if sums are supported. Finds ith number by total sum of numbers in the range [0, i], i.e.
  // finds such first number that sum of all number before and including it are equal or greater to sum.
  // sumIncl will contain the actual sum including found number, cntIncl contains count of numbers including
  // found one. Function returns found number.
  u64 FindBySum(u64 sum, u64 & sumIncl, u64 & cntIncl);
  // After setting position by FindByIndex and FindBySum functions Read() function will sequentially read
  // next number. It is only allowed to read numbers in same chunk as the first number found (one chunk is
  // created for one table entry).
  u64 Read();
private:
  void SetDecodeContext(u64 table_entry_index);
private:
  Reader & m_reader;
  u64 m_numsCnt;
  u64 m_numElemPerTableEntry;
  bool m_supportSums;
  u64 m_numsEncodedOffset;
  vector<u32> m_distrTable;
  vector<u64> m_tablePos;
  vector<u64> m_tableSum;
  // Decode context.
  struct DecodeContext;
  DecodeContext * m_decodeCtx;
};
