#pragma once
#include "../coding/endianness.hpp"
#include "../base/base.hpp"
#include "../base/assert.hpp"

class IntervalIndexBase
{
public:
#pragma pack(push, 1)
  struct Header
  {
    uint8_t m_CellIdLeafBytes;
  };

  struct Index
  {
    uint32_t m_BaseOffset;
    uint16_t m_Count[256];
  };
#pragma pack(pop)
};

template <typename ValueT, class ReaderT>
class IntervalIndex : public IntervalIndexBase
{
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
          ReadIndex(index.m_BaseOffset + (cumCount * sizeof(Index)), index1);
          ForEachImpl(f, b1, e1, index1, level - 1);
        }
        else
        {
          // TODO: Use binary search here if count is very large.
          uint32_t const step = sizeof(ValueT) + m_Header.m_CellIdLeafBytes;
          uint32_t const count = index.m_Count[i];
          uint32_t pos = index.m_BaseOffset + (cumCount * step);
          vector<char> data(step * count);
          char const * pData = &data[0];
          m_Reader.Read(pos, &data[0], data.size());
          for (uint32_t j = 0; j < count; ++j, pData += step)
          // for (uint32_t j = 0; j < count; ++j, pos += step)
          {
            Value value;
            value.m_CellId = 0;
            memcpy(&value, pData, step);
            // m_Reader.Read(pos, &value, step);
            uint32_t const cellId = SwapIfBigEndian(value.m_CellId);
            if (b1 <= cellId && cellId <= e1)
              f(SwapIfBigEndian(value.m_Value));
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
      index.m_BaseOffset = SwapIfBigEndian(index.m_BaseOffset);
      for (uint32_t i = 0; i < 256; ++i)
        index.m_Count[i] = SwapIfBigEndian(index.m_Count[i]);
    }
  }

#pragma pack(push, 1)
  struct Value
  {
    ValueT m_Value;
    uint32_t m_CellId;
  };
#pragma pack(pop)

  ReaderT m_Reader;
  Header m_Header;
  Index m_Level0Index;
  int m_CellIdBytes;
};
