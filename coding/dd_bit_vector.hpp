#pragma once
#include "dd_base.hpp"
#include "endianness.hpp"
#include "varint.hpp"
#include "../base/assert.hpp"
#include "../base/base.hpp"
#include "../base/bits.hpp"
#include "../std/type_traits.hpp"

#include "../base/start_mem_debug.hpp"

template <
    class TWordVector,
    typename TSize = typename TWordVector::size_type,
    typename TDifference = typename make_signed<TSize>::type
    > class DDBitVector
{
public:
  typedef TWordVector VectorType;
  typedef typename VectorType::value_type WordType;
  typedef TSize size_type;
  typedef TDifference difference_type;

  DDBitVector()
  {
  }

  template <class ReaderT>
  void Parse(DDParseInfo<ReaderT> & info)
  {
    Parse(info, ReadVarUint<size_type>(info.Source()));
  }

  template <class ReaderT>
  void Parse(DDParseInfo<ReaderT> & info, size_type vectorSize)
  {
    m_Size = vectorSize;
    m_Data = VectorType(info.Source().SubReader(size_words() * sizeof(WordType)), size_words());
  }

  bool operator[](size_type i) const
  {
    ASSERT(i < size(), (i, size()));
    return 0 !=
        (WordAt(i / 8 / sizeof(WordType)) & (WordType(1) << (i & (8 * sizeof(WordType) - 1))));
  }

  size_type size() const
  {
    return m_Size;
  }

  size_type size_words() const
  {
    return (m_Size + sizeof(WordType) * 8 - 1) / 8 / sizeof(WordType);
  }

  bool empty() const
  {
    return m_Size == 0;
  }

  WordType WordAt(size_type i) const
  {
    ASSERT(i < size_words(), (i));
    return SwapIfBigEndian(m_Data[i]);
  }

  size_type PopCountWords(size_type begWord, size_type endWord) const
  {
    if (begWord == endWord)
      return 0;
    ASSERT_LESS(begWord, endWord, ());
    ASSERT_LESS(begWord, size_words(), ());
    ASSERT_LESS_OR_EQUAL(endWord, size_words(), ());

    // popcount doesn't depend on byte order.
    size_type result = 0;
    while (begWord != endWord)
      result += bits::popcount(m_Data[begWord++]);
    return result;
  }

  size_type Select1FromWord(size_type iWord, size_type i) const
  {
    ASSERT(iWord < size_words(), (iWord, size_words(), i));
    size_type const startWord = iWord;
    size_type wordRank = bits::popcount(m_Data[iWord]);
    while (wordRank < i)
    {
      i -= wordRank;
      wordRank = bits::popcount(m_Data[++iWord]);
    }
    return (iWord - startWord) * sizeof(WordType) * 8 +
        bits::select1(SwapIfBigEndian(m_Data[iWord]), i);
  }

private:
  size_type m_Size;
  VectorType m_Data;
};

#include "../base/stop_mem_debug.hpp"
