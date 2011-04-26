#pragma once
#include "../coding/endianness.hpp"
#include "../coding/byte_stream.hpp"
#include "../coding/varint.hpp"
#include "../base/assert.hpp"
#include "../base/base.hpp"
#include "../base/bits.hpp"
#include "../base/buffer_vector.hpp"
#include "../base/macros.hpp"
#include "../std/memcpy.hpp"
#include "../std/static_assert.hpp"

class IntervalIndexBase
{
public:
#pragma pack(push, 1)
  struct Header
  {
    uint8_t m_Levels;
    uint8_t m_BitsPerLevel;
  };
#pragma pack(pop)
  STATIC_ASSERT(sizeof(Header) == 2);

  struct Index
  {
    inline uint32_t GetOffset() const { return m_Offset; }
    inline uint32_t Bit(uint32_t i) const { return (m_BitMask >> i) & 1; }

    uint32_t m_Offset;
    uint32_t m_BitMask;
  };
  STATIC_ASSERT(sizeof(Index) == 8);
};

template <class ReaderT>
class IntervalIndex : public IntervalIndexBase
{
public:

  class Query
  {
  public:
    void Clear() {}
  private:
    friend class IntervalIndex;
    vector<char> m_IntervalIndexCache;
  };

  explicit IntervalIndex(ReaderT const & reader)
    : m_Reader(reader)
  {
    m_Reader.Read(0, &m_Header, sizeof(m_Header));
    ASSERT_EQUAL(m_Header.m_BitsPerLevel, 5, ());
    ReadIndex(sizeof(m_Header), m_Level0Index);
  }

  template <typename F>
  void ForEach(F const & f, uint64_t beg, uint64_t end, Query & query) const
  {
    ASSERT_LESS(beg, 1ULL << m_Header.m_Levels * m_Header.m_BitsPerLevel, (beg, end));
    ASSERT_LESS_OR_EQUAL(end, 1ULL << m_Header.m_Levels * m_Header.m_BitsPerLevel, (beg, end));
    if (beg != end)
    {
      // end is inclusive in ForEachImpl().
      --end;
      ForEachImpl(f, beg, end, m_Level0Index, sizeof(m_Header) + sizeof(m_Level0Index),
                  (m_Header.m_Levels - 1) * m_Header.m_BitsPerLevel, query);
    }
  }

  template <typename F>
  void ForEach(F const & f, uint64_t beg, uint64_t end) const
  {
    Query query;
    ForEach(f, beg, end, query);
  }

private:
  template <typename F>
  void ForEachImpl(F const & f, uint64_t beg, uint64_t end, Index const & index,
                   uint32_t baseOffset, int skipBits, Query & query) const
  {
    uint32_t const beg0 = static_cast<uint32_t>(beg >> skipBits);
    uint32_t const end0 = static_cast<uint32_t>(end >> skipBits);
    ASSERT_GREATER_OR_EQUAL(skipBits, 0, (beg, end, baseOffset));
    ASSERT_LESS_OR_EQUAL(beg, end, (baseOffset, skipBits));
    ASSERT_LESS_OR_EQUAL(beg0, end0, (beg, end, baseOffset, skipBits));
    ASSERT_LESS(end0, (1 << m_Header.m_BitsPerLevel), (beg0, beg, end, baseOffset, skipBits));

    if (skipBits > 0)
    {
      uint32_t cumCount = 0;
      for (uint32_t i = 0; i < beg0; ++i)
        cumCount += index.Bit(i);
      for (uint32_t i = beg0; i <= end0; ++i)
      {
        if (index.Bit(i))
        {
          uint64_t const levelBytesFF = (1ULL << skipBits) - 1;
          uint64_t const b1 = (i == beg0) ? (beg & levelBytesFF) : 0;
          uint64_t const e1 = (i == end0) ? (end & levelBytesFF) : levelBytesFF;

          Index index1;
          uint32_t const offset = baseOffset + index.GetOffset() + (cumCount * sizeof(Index));
          ReadIndex(offset, index1);
          ForEachImpl(f, b1, e1, index1, offset + sizeof(Index),
                      skipBits - m_Header.m_BitsPerLevel, query);
          ++cumCount;
        }
      }
    }
    else
    {
      Index nextIndex;
      ReadIndex(baseOffset, nextIndex);
      uint32_t const begOffset = baseOffset + index.GetOffset();
      uint32_t const endOffset = baseOffset + sizeof(Index) + nextIndex.GetOffset();
      ASSERT_LESS(begOffset, endOffset, (beg, end, baseOffset, skipBits));
      buffer_vector<uint8_t, 256> data(endOffset - begOffset);
      m_Reader.Read(begOffset, &data[0], data.size());
      ArrayByteSource src(&data[0]);
      void const * const pEnd = &data[0] + data.size();
      uint32_t key = -1;
      uint32_t value = 0;
      while (src.Ptr() < pEnd)
      {
        uint32_t const x = ReadVarUint<uint32_t>(src);
        int32_t const delta = bits::ZigZagDecode(x >> 1);
        ASSERT_GREATER_OR_EQUAL(static_cast<int32_t>(value) + delta, 0, ());
        value += delta;
        if (x & 1)
          for (++key; key < (1 << m_Header.m_BitsPerLevel) && !index.Bit(key); ++key) ;
        ASSERT_LESS(key, 1 << m_Header.m_BitsPerLevel, (key));
        if (key > end0)
          break;
        if (key >= beg0)
          f(value);
      }
    }
  }

  void ReadIndex(uint64_t pos, Index & index) const
  {
    m_Reader.Read(pos, &index, sizeof(Index));
    index.m_Offset = SwapIfBigEndian(index.m_Offset);
    index.m_BitMask = SwapIfBigEndian(index.m_BitMask);
  }

  ReaderT m_Reader;
  Header m_Header;
  Index m_Level0Index;
  int m_CellIdBytes;
};
