#include "coding/varint_vector.hpp"

/*
#include "coding/writer.hpp"
#include "coding/reader.hpp"

#include "base/assert.hpp"


namespace varint
{

namespace
{
  void VarintEncode(vector<uint8_t> & dst, uint64_t n)
  {
    if (n == 0)
    {
      dst.push_back(0);
    }
    else
    {
      while (n != 0)
      {
        uint8_t b = n & 0x7F;
        n >>= 7;
        b |= (n == 0) ? 0 : 0x80;
        dst.push_back(b);
      }
    }
  }

  uint64_t VarintDecode(Reader * reader, uint64_t & offset)
  {
    uint64_t n = 0;
    int shift = 0;
    while (1)
    {
      uint8_t b = 0;
      reader->Read(offset, &b, 1);
      n |= uint64_t(b & 0x7F) << shift;
      ++offset;
      if ((b & 0x80) == 0)
        break;
      shift += 7;
    }
    return n;
  }
}

VectorBuilder::VectorBuilder(uint64_t numElemPerTableEntry)
  : m_numElemPerTableEntry(numElemPerTableEntry), m_numsCount(0), m_sum(0)
{
}

void VectorBuilder::AddNum(uint64_t num)
{
  if (m_numsCount % m_numElemPerTableEntry == 0)
  {
    TableEntry tableEntry;
    tableEntry.pos = m_serialNums.size();
    tableEntry.sum = m_sum;
    m_selectTable.push_back(tableEntry);
  }
  VarintEncode(m_serialNums, num);
  ++m_numsCount;
  m_sum += num;
}

void VectorBuilder::Finalize(Writer * writer)
{
  vector<uint8_t> header;
  VarintEncode(header, m_numsCount);
  VarintEncode(header, m_numElemPerTableEntry);
  VarintEncode(header, m_selectTable.size());
  VarintEncode(header, m_serialNums.size());

  writer->Write(header.data(), header.size());
  writer->Write(m_selectTable.data(), m_selectTable.size() * sizeof(m_selectTable.front()));
  writer->Write(m_serialNums.data(), m_serialNums.size());
}


void VectorBuilderDelayedLast::AddLast()
{
  if (m_hasLast)
  {
    BaseT::AddNum(m_last);
    m_hasLast = false;
  }
}

void VectorBuilderDelayedLast::AddNum(uint64_t num)
{
  AddLast();

  m_last = num;
  m_hasLast = true;
}

void VectorBuilderDelayedLast::ReplaceLast(uint64_t num)
{
  ASSERT(m_hasLast, ());
  m_last = num;
}

void VectorBuilderDelayedLast::Finalize(Writer * writer)
{
  AddLast();

  BaseT::Finalize(writer);
}


Vector::Vector(Reader * reader)
  : m_reader(reader), m_numsCount(0), m_numElemPerTableEntry(0), m_numTableEntries(0),
    m_serialNumsSize(0), m_selectTableOffset(0), m_serialNumsOffset(0)
{
  uint64_t parseOffset = 0;
  m_numsCount = VarintDecode(m_reader, parseOffset);
  m_numElemPerTableEntry = VarintDecode(m_reader, parseOffset);
  m_numTableEntries = VarintDecode(m_reader, parseOffset);
  m_serialNumsSize = VarintDecode(m_reader, parseOffset);
  m_selectTableOffset = parseOffset;
  m_serialNumsOffset = m_selectTableOffset + sizeof(TableEntry) * m_numTableEntries;
}

void Vector::FindByIndex(uint64_t index, uint32_t & serialPos, uint64_t & sumBefore)
{
  ASSERT_LESS(index, m_numsCount, ());
  uint64_t tableEntryIndex = index / m_numElemPerTableEntry;

  ASSERT_LESS(tableEntryIndex, m_numTableEntries, ());

  uint64_t tableEntryOffset = m_selectTableOffset + tableEntryIndex * sizeof(TableEntry);
  uint64_t indexWithinRange = index % m_numElemPerTableEntry;

  TableEntry tableEntry;
  m_reader->Read(tableEntryOffset, &tableEntry, sizeof(TableEntry));

  uint64_t sum = tableEntry.sum;
  uint64_t numOffset = m_serialNumsOffset + tableEntry.pos;
  for (uint64_t i = 0; i < indexWithinRange; ++i)
  {
    uint64_t num = VarintDecode(m_reader, numOffset);
    sum += num;
  }
  serialPos = numOffset - m_serialNumsOffset;
  sumBefore = sum;
}

void Vector::FindBySum(uint64_t sum, uint32_t & serialPos, uint64_t & sumBefore, uint64_t & countBefore)
{
  // First do binary search over select table to find the biggest
  // sum that is less or equal to our.
  uint64_t l = 0, r = m_numTableEntries;
  uint64_t countBinarySearchCycles = 0;
  while (r - l > 1)
  {
    ++countBinarySearchCycles;

    uint64_t m = (l + r) / 2;
    uint64_t tableEntryOffset = m_selectTableOffset + m * sizeof(TableEntry);

    TableEntry tableEntry;
    m_reader->Read(tableEntryOffset, &tableEntry, sizeof(TableEntry));
    if (sum >= tableEntry.sum)
      l = m;
    else
      r = m;
  }

  uint64_t tableEntryIndex = l;
  countBefore = tableEntryIndex * m_numElemPerTableEntry;

  uint64_t tableEntryOffset = m_selectTableOffset + tableEntryIndex * sizeof(TableEntry);
  TableEntry tableEntry;
  m_reader->Read(tableEntryOffset, &tableEntry, sizeof(TableEntry));

  uint64_t numsSum = tableEntry.sum;
  // At this point nums_sum <= sum.
  uint64_t numOffset = m_serialNumsOffset + tableEntry.pos;
  while (numsSum <= sum && countBefore < m_numsCount)
  {
    uint64_t nextOffset = numOffset;
    uint64_t num = VarintDecode(m_reader, nextOffset);

    if (numsSum + num > sum)
      break;

    numOffset = nextOffset;
    numsSum += num;
    ++countBefore;
  }

  serialPos = numOffset - m_serialNumsOffset;
  sumBefore = numsSum;
}

void Vector::Read(uint32_t & serialPos, uint64_t & num)
{
  ASSERT_LESS(serialPos, m_serialNumsSize, ());

  uint64_t numOffset = m_serialNumsOffset + serialPos;
  num = VarintDecode(m_reader, numOffset);
  serialPos = numOffset - m_serialNumsOffset;
}

}
*/
