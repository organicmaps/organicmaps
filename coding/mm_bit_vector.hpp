#pragma once
#include "endianness.hpp"
#include "mm_base.hpp"
#include "../base/assert.hpp"
#include "../base/base.hpp"
#include "../base/bits.hpp"
#include "../std/algorithm.hpp"
#include <boost/iterator/transform_iterator.hpp>

#include "../base/start_mem_debug.hpp"

//  .       !!
//    network byte order (big endian).
//       .
template <typename TWord> class MMBitVector
{
public:
  typedef TWord WordType;

  MMBitVector()
  {
  }

  MMBitVector(void const *p, size_t size)
  {
    MMParseInfo parseInfo(p, size, true);
    this->Parse(parseInfo);
  }

  bool operator[](TWord i) const
  {
    ASSERT(i < size(), (i, size()));
    return (WordAt(i / 8 / sizeof(TWord)) & (TWord(1) << (i & (8 * sizeof(TWord) - 1)))) != 0;
  }

  TWord size() const
  {
    return m_Size;
  }

  TWord size_words() const
  {
    return (m_Size + sizeof(TWord)*8 - 1) / 8 / sizeof(TWord);
  }

  size_t bytes_used() const
  {
    return (size_words() + 1) * sizeof(TWord);
  }

  bool empty() const
  {
    return m_Size == 0;
  }

  TWord WordAt(TWord i) const
  {
    ASSERT(i < size_words(), (i));
    return SwapIfBigEndian(m_pWords[i]);
  }

  TWord Select1FromWord(TWord word, TWord i) const
  {
    ASSERT(word < size_words(), (word, size_words(), i));
    TWord const * const pStartWord = m_pWords + word;
    TWord const * pWord = pStartWord;
    TWord wordRank = bits::popcount(*pWord);
    while (wordRank < i)
    {
      i -= wordRank;
      wordRank = bits::popcount(*++pWord);
    }
    return static_cast<TWord>(pWord - pStartWord) * sizeof(TWord) * 8 +
        bits::select1(SwapIfBigEndian(*pWord), i);
  }

  TWord PopCountWords(TWord begWord, TWord endWord) const
  {
    //  popcount   byte order.
    ASSERT(begWord <  size_words(), (begWord));
    ASSERT(endWord <= size_words(), (endWord));
    return bits::popcount(m_pWords + begWord, endWord - begWord);
  }

  void Parse(MMParseInfo & info)
  {
    if (!info.Successful()) return;
    TWord size = *info.Advance<TWord>(1);
    Parse(info, SwapIfBigEndian(size));
  }

  void Parse(MMParseInfo & info, TWord vectorSize)
  {
    m_Size = vectorSize;
    if (!info.Successful()) return;
    m_pWords = info.Advance<TWord>(static_cast<size_t>(size_words()));
  }

private:
  TWord const * m_pWords;
  TWord m_Size;
};

//     ,   Rank0  Rank1
// Select0  Select1  .
class MMBitVector32RankDirectory
{
public:
  MMBitVector32RankDirectory(MMBitVector<uint32_t> const & bitVector) : m_BitVector(bitVector)
  {
  }

  MMBitVector32RankDirectory(MMBitVector<uint32_t> const & bitVector, void const * p, size_t size)
    : m_BitVector(bitVector)
  {
    MMParseInfo parseInfo(p, size, true);
    this->Parse(parseInfo);
  }

  uint32_t Rank0(uint32_t x) const
  {
    ASSERT(x < m_BitVector.size(), (x, m_BitVector.size()));
    return x + 1 - Rank1(x);
  }

  uint32_t Rank1(uint32_t x) const
  {
    ASSERT(x < m_BitVector.size(), (x, m_BitVector.size()));
    uint32_t const iWord = x >> 5;
    uint32_t const iChunk = iWord >> m_LogChunkSize;
    return (iChunk == 0 ? 0 : SwapIfBigEndian(m_pChunks[iChunk - 1])) +
           m_BitVector.PopCountWords(iChunk << m_LogChunkSize, iWord) +
           bits::popcount(m_BitVector.WordAt(iWord) & (0xFFFFFFFFU >> (31 - (x - (iWord << 5)))));
  }

  uint32_t Select1(uint32_t i) const
  {
    ASSERT(i > 0 && i <= m_MaxRank, (i, m_MaxRank));
    // TODO: First try approximate lower and upper bound.
    uint32_t iChunk = lower_bound(
        boost::make_transform_iterator(m_pChunks, &SwapIfBigEndian<uint32_t>),
        boost::make_transform_iterator(m_pChunks + size_chunks(), &SwapIfBigEndian<uint32_t>),
        i) - boost::make_transform_iterator(m_pChunks, &SwapIfBigEndian<uint32_t>);
    ASSERT_LESS(iChunk, size_chunks(), ());
    ASSERT_GREATER_OR_EQUAL(SwapIfBigEndian(m_pChunks[iChunk]), i, ());
    ASSERT_LESS((iChunk << m_LogChunkSize), m_BitVector.size_words(), (iChunk, m_LogChunkSize));
    return (iChunk << (5 + m_LogChunkSize)) +
        m_BitVector.Select1FromWord(
            iChunk << m_LogChunkSize,
            i - (iChunk == 0 ? 0 : SwapIfBigEndian(m_pChunks[iChunk - 1])));
  }

  uint32_t size_chunks() const
  {
    return bits::RoundLastBitsUpAndShiftRight(m_BitVector.size(), 5 + m_LogChunkSize);
  }

  size_t bytes_used() const
  {
    return (1 + size_chunks()) << (2 + m_LogChunkSize);
  }

  void Parse(MMParseInfo & info)
  {
    // TODO: Store version in MMBitVector32RankDirectory?
    if (!info.Successful()) return;
    m_LogChunkSize = SwapIfBigEndian(*info.Advance<uint32_t>());
    if (!info.Successful()) return;
    m_pChunks = info.Advance<uint32_t>(size_chunks());
#ifdef DEBUG
    m_MaxRank = (m_BitVector.empty() ? 0 : Rank1(m_BitVector.size() - 1));
    if (m_MaxRank > m_BitVector.size())
    {
      CHECK(!info.FailOnError(), (m_MaxRank, m_BitVector.size()));
      info.Fail();
    }
#endif
  }

protected:
  MMBitVector<uint32_t> const & m_BitVector;
  uint32_t const * m_pChunks;
  uint32_t m_LogChunkSize;             //    32  .
#ifdef DEBUG
  uint32_t m_MaxRank;
#endif
};

#include "../base/stop_mem_debug.hpp"
