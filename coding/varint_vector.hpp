#pragma once

/*
#include "base/base.hpp"

#include "std/vector.hpp"


class Writer;
class Reader;

namespace varint
{

#pragma pack(push, 1)
struct TableEntry
{
  uint32_t pos;
  uint64_t sum;
};
#pragma pack(pop)


class VectorBuilder
{
protected:
  // Implicit expectation: total compressed size should be within 4GB.
  static uint64_t const DEF_NUM_ELEMENTS_PER_TABLE_ENTRY = 1024;

public:
  VectorBuilder(uint64_t numElemPerTableEntry = DEF_NUM_ELEMENTS_PER_TABLE_ENTRY);

  void AddNum(uint64_t num);
  void Finalize(Writer * writer);

protected:
  uint64_t m_numElemPerTableEntry;
  uint64_t m_numsCount;
  uint64_t m_sum;
  vector<TableEntry> m_selectTable;
  vector<uint8_t> m_serialNums;
};

class VectorBuilderDelayedLast : public VectorBuilder
{
  typedef VectorBuilder BaseT;
  uint64_t m_last;
  bool m_hasLast;

  void AddLast();
public:
  VectorBuilderDelayedLast(uint64_t numElemPerTableEntry = DEF_NUM_ELEMENTS_PER_TABLE_ENTRY)
    : BaseT(numElemPerTableEntry), m_hasLast(false)
  {
  }

  void AddNum(uint64_t num);
  void ReplaceLast(uint64_t num);
  void Finalize(Writer * writer);

  bool HasLast() const { return m_hasLast; }
  uint64_t GetLast() const { return m_last; }
  uint64_t GetNumsCount() const { return m_numsCount + (m_hasLast ? 1 : 0); }
};

class Vector
{
public:
  Vector(Reader * reader);

  void FindByIndex(uint64_t countBefore, uint32_t & serialPos, uint64_t & sumBefore);
  void FindBySum(uint64_t sum, uint32_t & serialPos, uint64_t & sumBefore, uint64_t & countBefore);
  void Read(uint32_t & serialPos, uint64_t & num);

private:
  Reader * m_reader;
  uint64_t m_numsCount;
  uint64_t m_numElemPerTableEntry;
  uint64_t m_numTableEntries;
  uint64_t m_serialNumsSize;
  uint64_t m_selectTableOffset;
  uint64_t m_serialNumsOffset;
};

}
*/
