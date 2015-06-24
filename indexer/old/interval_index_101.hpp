#pragma once
#include "indexer/interval_index_iface.hpp"

#include "coding/endianness.hpp"

#include "base/assert.hpp"
#include "base/base.hpp"
#include "base/macros.hpp"

#include "std/cstring.hpp"


namespace old_101 {

class IntervalIndexBase : public IntervalIndexIFace
{
public:
#pragma pack(push, 1)
  struct Header
  {
    uint8_t m_CellIdLeafBytes;
  };
#pragma pack(pop)

  class Index
  {
  public:
    uint32_t GetBaseOffset() const { return UINT32_FROM_UINT16(m_BaseOffsetHi, m_BaseOffsetLo); }
    void SetBaseOffset(uint32_t baseOffset)
    {
      m_BaseOffsetLo = UINT32_LO(baseOffset);
      m_BaseOffsetHi = UINT32_HI(baseOffset);
    }

  private:
    uint16_t m_BaseOffsetLo;
    uint16_t m_BaseOffsetHi;
  public:
    uint16_t m_Count[256];
  };
  static_assert(sizeof(Index) == 2 * 258, "");
};

// TODO: IntervalIndex shouldn't do SwapIfBigEndian for ValueT.
template <typename ValueT, class ReaderT>
class IntervalIndex : public IntervalIndexBase
{
  typedef IntervalIndexBase base_t;

public:

  IntervalIndex(ReaderT const & reader, int cellIdBytes = 5)
    : m_Reader(reader), m_CellIdBytes(cellIdBytes)
  {
    m_Reader.Read(0, &m_Header, sizeof(m_Header));
    ReadIndex(sizeof(m_Header), m_Level0Index);
  }

  template <typename F>
  void ForEach(F const & f, uint64_t beg, uint64_t end) const
  {
    ASSERT_LESS(beg, 1ULL << 8 * m_CellIdBytes, (beg, end));
    ASSERT_LESS_OR_EQUAL(end, 1ULL << 8 * m_CellIdBytes, (beg, end));
    // end is inclusive in ForEachImpl().
    --end;
    ForEachImpl(f, beg, end, m_Level0Index, m_CellIdBytes - 1);
  }

  virtual void DoForEach(FunctionT const & f, uint64_t beg, uint64_t end)
  {
    ForEach(f, beg, end);
  }

private:
  template <typename F>
  void ForEachImpl(F const & f, uint64_t beg, uint64_t end, Index const & index, int level) const
  {
    uint32_t const beg0 = static_cast<uint32_t>(beg >> (8 * level));
    uint32_t const end0 = static_cast<uint32_t>(end >> (8 * level));
    uint32_t cumCount = 0;
    for (uint32_t i = 0; i < beg0; ++i)
      cumCount += index.m_Count[i];
    for (uint32_t i = beg0; i <= end0; ++i)
    {
      ASSERT_LESS(i, 256, ());
      if (index.m_Count[i] != 0)
      {
        uint64_t const levelBytesFF = (1ULL << 8 * level) - 1;
        uint64_t const b1 = (i == beg0) ? (beg & levelBytesFF) : 0;
        uint64_t const e1 = (i == end0) ? (end & levelBytesFF) : levelBytesFF;
        if (level > m_Header.m_CellIdLeafBytes)
        {
          Index index1;
          ReadIndex(index.GetBaseOffset() + (cumCount * sizeof(Index)), index1);
          ForEachImpl(f, b1, e1, index1, level - 1);
        }
        else
        {
          // TODO: Use binary search here if count is very large.
          uint32_t const step = sizeof(ValueT) + m_Header.m_CellIdLeafBytes;
          uint32_t const count = index.m_Count[i];
          uint32_t pos = index.GetBaseOffset() + (cumCount * step);
          size_t const readSize = step * count;
          vector<char> dataCache(readSize, 0);
          char * pData = &dataCache[0];
          m_Reader.Read(pos, pData, readSize);
          for (uint32_t j = 0; j < count; ++j, pData += step)
          // for (uint32_t j = 0; j < count; ++j, pos += step)
          {
            ValueT value;
            uint32_t cellIdOnDisk = 0;
            memcpy(&value, pData, sizeof(ValueT));
            memcpy(&cellIdOnDisk, pData + sizeof(ValueT), m_Header.m_CellIdLeafBytes);
            // m_Reader.Read(pos, &value, step);
            uint32_t const cellId = SwapIfBigEndian(cellIdOnDisk);
            if (b1 <= cellId && cellId <= e1)
              f(SwapIfBigEndian(value));
          }
        }
        cumCount += index.m_Count[i];
      }
    }
  }

  void ReadIndex(uint64_t pos, Index & index) const
  {
    m_Reader.Read(pos, &index, sizeof(Index));
    if (IsBigEndian())
    {
      for (uint32_t i = 0; i < 256; ++i)
        index.m_Count[i] = SwapIfBigEndian(index.m_Count[i]);
    }
  }

  ReaderT m_Reader;
  Header m_Header;
  Index m_Level0Index;
  int m_CellIdBytes;
};

}
