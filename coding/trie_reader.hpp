#pragma once
#include "coding/trie.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "base/assert.hpp"
#include "base/bits.hpp"
#include "base/macros.hpp"

namespace trie
{
template <class ValueReaderT, typename EdgeValueT>
class LeafIterator0 : public Iterator<typename ValueReaderT::ValueType, EdgeValueT>
{
public:
  typedef typename ValueReaderT::ValueType ValueType;
  typedef EdgeValueT EdgeValueType;

  template <class ReaderT>
  LeafIterator0(ReaderT const & reader, ValueReaderT const & valueReader)
  {
    uint32_t const size = static_cast<uint32_t>(reader.Size());
    ReaderSource<ReaderT> src(reader);
    while (src.Pos() < size)
    {
      this->m_value.push_back(ValueType());
#ifdef DEBUG
      uint64_t const pos = src.Pos();
#endif
      valueReader(src, this->m_value.back());
      ASSERT_NOT_EQUAL(pos, src.Pos(), ());
    }
    ASSERT_EQUAL(size, src.Pos(), ());
  }

  Iterator<ValueType, EdgeValueType> * Clone() const
  {
    return new LeafIterator0<ValueReaderT, EdgeValueT>(*this);
  }

  Iterator<ValueType, EdgeValueType> * GoToEdge(size_t i) const
  {
    ASSERT(false, (i));
    UNUSED_VALUE(i);
    return NULL;
  }
};

template <class ReaderT, class ValueReaderT, class EdgeValueReaderT>
class IteratorImplBase :
    public Iterator<typename ValueReaderT::ValueType, typename EdgeValueReaderT::ValueType>
{
protected:
  enum { IS_READER_IN_MEMORY = 0 };
};

template <class ValueReaderT, class EdgeValueReaderT>
class IteratorImplBase<SharedMemReader, ValueReaderT, EdgeValueReaderT> :
    public Iterator<typename ValueReaderT::ValueType, typename EdgeValueReaderT::ValueType>
{
protected:
  enum { IS_READER_IN_MEMORY = 1 };
};


template <class ReaderT, class ValueReaderT, class EdgeValueReaderT>
class Iterator0 : public IteratorImplBase<ReaderT, ValueReaderT, EdgeValueReaderT>
{
public:
  typedef typename ValueReaderT::ValueType ValueType;
  typedef typename EdgeValueReaderT::ValueType EdgeValueType;

  Iterator0(ReaderT const & reader,
            ValueReaderT const & valueReader,
            EdgeValueReaderT const &  edgeValueReader,
            TrieChar baseChar)
    : m_reader(reader), m_valueReader(valueReader), m_edgeValueReader(edgeValueReader)
  {
    ParseNode(baseChar);
  }

  Iterator<ValueType, EdgeValueType> * Clone() const
  {
    return new Iterator0<ReaderT, ValueReaderT, EdgeValueReaderT>(*this);
  }

  Iterator<ValueType, EdgeValueType> * GoToEdge(size_t i) const
  {
    ASSERT_LESS(i, this->m_edge.size(), ());
    uint32_t const offset = m_edgeInfo[i].m_offset;
    uint32_t const size = m_edgeInfo[i+1].m_offset - offset;

    // TODO: Profile to check that MemReader optimization helps?
    /*
    if (!IteratorImplBase<ReaderT, ValueReaderT, EdgeValueReaderT>::IS_READER_IN_MEMORY &&
        size < 1024)
    {
      SharedMemReader memReader(size);
      m_reader.Read(offset, memReader.Data(), size);
      if (m_edgeInfo[i].m_isLeaf)
        return new LeafIterator0<SharedMemReader, ValueReaderT, EdgeValueType>(
              memReader, m_valueReader);
      else
        return new Iterator0<SharedMemReader, ValueReaderT, EdgeValueReaderT>(
              memReader, m_valueReader, m_edgeValueReader,
              this->m_edge[i].m_str.back());
    }
    else
    */
    {
      if (m_edgeInfo[i].m_isLeaf)
        return new LeafIterator0<ValueReaderT, EdgeValueType>(
              m_reader.SubReader(offset, size), m_valueReader);
      else
        return new Iterator0<ReaderT, ValueReaderT, EdgeValueReaderT>(
              m_reader.SubReader(offset, size), m_valueReader, m_edgeValueReader,
              this->m_edge[i].m_str.back());
    }
  }

private:
  void ParseNode(TrieChar baseChar)
  {
    ReaderSource<ReaderT> src(m_reader);

    // [1: header]: [2: min(valueCount, 3)] [6: min(childCount, 63)]
    uint8_t const header = ReadPrimitiveFromSource<uint8_t>(src);
    uint32_t valueCount = (header >> 6);
    uint32_t childCount = (header & 63);

    // [vu valueCount]: if valueCount in header == 3
    if (valueCount == 3)
      valueCount = ReadVarUint<uint32_t>(src);

    // [vu childCount]: if childCount in header == 63
    if (childCount == 63)
      childCount = ReadVarUint<uint32_t>(src);

    // [value] ... [value]
    this->m_value.resize(valueCount);
    for (uint32_t i = 0; i < valueCount; ++i)
      m_valueReader(src, this->m_value[i]);

    // [childInfo] ... [childInfo]
    this->m_edge.resize(childCount);
    m_edgeInfo.resize(childCount + 1);
    m_edgeInfo[0].m_offset = 0;
    for (uint32_t i = 0; i < childCount; ++i)
    {
      typename Iterator<ValueType, EdgeValueType>::Edge & e = this->m_edge[i];

      // [1: header]: [1: isLeaf] [1: isShortEdge] [6: (edgeChar0 - baseChar) or min(edgeLen-1, 63)]
      uint8_t const header = ReadPrimitiveFromSource<uint8_t>(src);
      m_edgeInfo[i].m_isLeaf = ((header & 128) != 0);
      if (header & 64)
        e.m_str.push_back(baseChar + bits::ZigZagDecode(header & 63U));
      else
      {
        // [vu edgeLen-1]: if edgeLen-1 in header == 63
        uint32_t edgeLen = (header & 63);
        if (edgeLen == 63)
          edgeLen = ReadVarUint<uint32_t>(src);
        edgeLen += 1;

        // [vi edgeChar0 - baseChar] [vi edgeChar1 - edgeChar0] ... [vi edgeCharN - edgeCharN-1]
        e.m_str.reserve(edgeLen);
        for (uint32_t i = 0; i < edgeLen; ++i)
          e.m_str.push_back(baseChar += ReadVarInt<int32_t>(src));
      }

      // [edge value]
      m_edgeValueReader(src, e.m_value);

      // [child size]: if the child is not the last one
      m_edgeInfo[i + 1].m_offset = m_edgeInfo[i].m_offset;
      if (i != childCount - 1)
        m_edgeInfo[i + 1].m_offset += ReadVarUint<uint32_t>(src);

      baseChar = e.m_str[0];
    }

    uint32_t const currentOffset = static_cast<uint32_t>(src.Pos());
    for (size_t i = 0; i < m_edgeInfo.size(); ++i)
      m_edgeInfo[i].m_offset += currentOffset;
    m_edgeInfo.back().m_offset = static_cast<uint32_t>(m_reader.Size());
  }

  struct EdgeInfo
  {
    uint32_t m_offset;
    bool m_isLeaf;
  };

  buffer_vector<EdgeInfo, 9> m_edgeInfo;

  ReaderT m_reader;
  ValueReaderT m_valueReader;
  EdgeValueReaderT m_edgeValueReader;
};

// Returns iterator to the root of the trie.
template <class ReaderT, class ValueReaderT, class EdgeValueReaderT>
Iterator<typename ValueReaderT::ValueType, typename EdgeValueReaderT::ValueType> *
ReadTrie(ReaderT const & reader,
         ValueReaderT valueReader = ValueReaderT(),
         EdgeValueReaderT edgeValueReader = EdgeValueReaderT())
{
  return new Iterator0<ReaderT, ValueReaderT, EdgeValueReaderT>(
        reader, valueReader, edgeValueReader, DEFAULT_CHAR);
}

}  // namespace trie
