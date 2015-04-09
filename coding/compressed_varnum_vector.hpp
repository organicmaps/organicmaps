// Author: Artyom.
// A module for storing arbitrary variable-bitsize numbers in a compressed form so that later
// you can access any number searching it by index or sum of numbers preceeding and including searched number.

#pragma once

/*
#include "std/function.hpp"
#include "std/stdint.hpp"
#include "std/vector.hpp"

// Forward declarations.
class Reader;
class Writer;

// Number of nums in a chunk per one table entry.
uint64_t const NUM_ELEM_PER_TABLE_ENTRY = 1024;

// A source of nums.
typedef function<uint64_t (uint64_t pos)> NumsSourceFuncT;
// Builds CompressedVarnumVector based on source of numbers.
// If supportSums is true then sums are included in the table otherwise sums are not computed.
void BuildCompressedVarnumVector(Writer & writer, NumsSourceFuncT numsSource, uint64_t numsCnt, bool supportSums);

// Reader of CompressedVarnumVector.
class CompressedVarnumVectorReader
{
public:
  // Bytes are read from Reader on the flight while decoding.
  CompressedVarnumVectorReader(Reader & reader);
  ~CompressedVarnumVectorReader();

  // Set current number decoding context to number at given index.
  // sumBefore will contain total sum of numbers before indexed number, computed only if sums are supported.
  void FindByIndex(uint64_t index, uint64_t & sumBefore);
  // Works only if sums are supported. Finds ith number by total sum of numbers in the range [0, i], i.e.
  // finds such first number that sum of all number before and including it are equal or greater to sum.
  // sumIncl will contain the actual sum including found number, cntIncl contains count of numbers including
  // found one. Function returns found number.
  uint64_t FindBySum(uint64_t sum, uint64_t & sumIncl, uint64_t & cntIncl);
  // After setting position by FindByIndex and FindBySum functions Read() function will sequentially read
  // next number. It is only allowed to read numbers in same chunk as the first number found (one chunk is
  // created for one table entry).
  uint64_t Read();
private:
  void SetDecodeContext(uint64_t table_entry_index);
private:
  Reader & m_reader;
  uint64_t m_numsCnt;
  uint64_t m_numElemPerTableEntry;
  bool m_supportSums;
  uint64_t m_numsEncodedOffset;
  vector<uint32_t> m_distrTable;
  vector<uint64_t> m_tablePos;
  vector<uint64_t> m_tableSum;
  // Decode context.
  struct DecodeContext;
  DecodeContext * m_decodeCtx;
};
*/
