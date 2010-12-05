#pragma once
#include "dd_base.hpp"
#include "dd_vector.hpp"
#include "endianness.hpp"
#include "varint.hpp"
#include "../base/assert.hpp"
#include "../base/bits.hpp"
#include <boost/iterator/transform_iterator.hpp>
#include "../base/start_mem_debug.hpp"

template <class TBitVector, class SizeT = typename TBitVector::size_type> class DDBitRankDirectory
{
public:
  typedef TBitVector BitVectorType;
  typedef SizeT size_type;

  DDBitRankDirectory()
  {
  }

  template <class ReaderT>
  void Parse(DDParseInfo<ReaderT> & info)
  {
    BitVectorType bitVector;
    bitVector.Parse(info);
    Parse(info, bitVector);
  }

  template <class ReaderT>
  void Parse(DDParseInfo<ReaderT> & info, BitVectorType const & bitVector)
  {
    Parse(info, bitVector, ReadVarUint<size_type>(info.Source()));
  }

  template <class ReaderT>
  void Parse(DDParseInfo<ReaderT> &info, BitVectorType const & bitVector, size_type logChunkSize)
  {
    m_BitVector = bitVector;
    m_LogChunkSize = logChunkSize;
    size_type const sizeChunks = bits::RoundLastBitsUpAndShiftRight(
        m_BitVector.size(), bits::LogBitSizeOfType<size_type>::value + m_LogChunkSize);
    if (sizeChunks != 0)
    {
      m_Chunks = ChunkVectorType(info.Source().SubReader(sizeChunks * sizeof(size_type)),
                                 sizeChunks);
      if (SwapIfBigEndian(m_Chunks[m_Chunks.size() - 1]) > m_BitVector.size())
        MYTHROW(DDParseException, (m_Chunks.size(),
                                   SwapIfBigEndian(m_Chunks[m_Chunks.size() - 1]),
                                   m_BitVector.size()));
    }
  }

  size_type Rank0(size_type x) const
  {
    ASSERT_LESS(x, m_BitVector.size(), ());
    return x + 1 - Rank1(x);
  }

  size_type Rank1(size_type x) const
  {
    ASSERT_LESS(x, m_BitVector.size(), ());
    size_type const logBitSize = bits::LogBitSizeOfType<size_type>::value;
    size_type const iWord = x >> logBitSize;
    size_type const iChunk = iWord >> m_LogChunkSize;
    return (iChunk == 0 ? 0 : SwapIfBigEndian(m_Chunks[iChunk - 1])) +
           m_BitVector.PopCountWords(iChunk << m_LogChunkSize, iWord) +
           bits::popcount(
               m_BitVector.WordAt(iWord) &
               (size_type(-1) >> ((8 * sizeof(size_type) - 1) - (x - (iWord << logBitSize)))));
  }

  size_type Select1(size_type i) const
  {
    ASSERT_GREATER(i, 0, ());
    ASSERT_LESS_OR_EQUAL(i, m_Chunks[m_Chunks.size() - 1], ());
    // TODO: First try approximate lower and upper bound.
    size_type iChunk = lower_bound(
        boost::make_transform_iterator(m_Chunks.begin(),
                                       &SwapIfBigEndian<size_type>),
        boost::make_transform_iterator(m_Chunks.end(),
                                       &SwapIfBigEndian<size_type>),
        i) -
        boost::make_transform_iterator(m_Chunks.begin(),
                                       &SwapIfBigEndian<size_type>);
    ASSERT_LESS(iChunk, m_Chunks.size(), ());
    ASSERT_GREATER_OR_EQUAL(SwapIfBigEndian(m_Chunks[iChunk]), i, ());
    ASSERT_LESS((iChunk << m_LogChunkSize), m_BitVector.size_words(), (iChunk, m_LogChunkSize));
    return (iChunk << (bits::LogBitSizeOfType<size_type>::value + m_LogChunkSize)) +
        m_BitVector.Select1FromWord(
            iChunk << m_LogChunkSize,
            i - (iChunk == 0 ? 0 : SwapIfBigEndian(m_Chunks[iChunk - 1])));
  }

  size_type size() const
  {
    return m_BitVector.size();
  }

  bool empty() const
  {
    return size() == 0;
  }

  bool operator[](size_type i) const
  {
    return m_BitVector[i];
  }

  TBitVector const & BitVector() const
  {
    return m_BitVector;
  }

private:
  TBitVector m_BitVector;
  typedef DDVector<size_type, typename TBitVector::VectorType::ReaderType> ChunkVectorType;
  ChunkVectorType m_Chunks;
  size_type m_LogChunkSize; // Chunk size in size_types.
};

#include "../base/stop_mem_debug.hpp"
