#pragma once
#include "coding/trie.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"

#include "base/assert.hpp"
#include "base/bits.hpp"
#include "base/macros.hpp"

namespace trie
{
template <class TValueReader>
class LeafIterator0 : public Iterator<typename TValueReader::ValueType>
{
public:
  using ValueType = typename TValueReader::ValueType;

  template <class TReader>
  LeafIterator0(TReader const & reader, TValueReader const & valueReader)
  {
    uint32_t const size = static_cast<uint32_t>(reader.Size());
    ReaderSource<TReader> src(reader);
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

  // trie::Iterator overrides:
  unique_ptr<Iterator<ValueType>> Clone() const override
  {
    return make_unique<LeafIterator0<TValueReader>>(*this);
  }

  unique_ptr<Iterator<ValueType>> GoToEdge(size_t i) const override
  {
    ASSERT(false, (i));
    UNUSED_VALUE(i);
    return nullptr;
  }
};

template <class TReader, class TValueReader>
class IteratorImplBase : public Iterator<typename TValueReader::ValueType>
{
protected:
  enum { IS_READER_IN_MEMORY = 0 };
};

template <class TValueReader>
class IteratorImplBase<SharedMemReader, TValueReader>
    : public Iterator<typename TValueReader::ValueType>
{
protected:
  enum { IS_READER_IN_MEMORY = 1 };
};

template <class TReader, class TValueReader>
class Iterator0 : public IteratorImplBase<TReader, TValueReader>
{
public:
  typedef typename TValueReader::ValueType ValueType;

  Iterator0(TReader const & reader, TValueReader const & valueReader, TrieChar baseChar)
    : m_reader(reader), m_valueReader(valueReader)
  {
    ParseNode(baseChar);
  }

  // trie::Iterator overrides:
  unique_ptr<Iterator<ValueType>> Clone() const override
  {
    return make_unique<Iterator0<TReader, TValueReader>>(*this);
  }

  unique_ptr<Iterator<ValueType>> GoToEdge(size_t i) const override
  {
    ASSERT_LESS(i, this->m_edge.size(), ());
    uint32_t const offset = m_edgeInfo[i].m_offset;
    uint32_t const size = m_edgeInfo[i+1].m_offset - offset;

    // TODO: Profile to check that MemReader optimization helps?
    /*
    if (!IteratorImplBase<TReader, TValueReader>::IS_READER_IN_MEMORY &&
        size < 1024)
    {
      SharedMemReader memReader(size);
      m_reader.Read(offset, memReader.Data(), size);
      if (m_edgeInfo[i].m_isLeaf)
        return make_unique<LeafIterator0<SharedMemReader, TValueReader>>(
              memReader, m_valueReader);
      else
        return make_unique<Iterator0<SharedMemReader, TValueReader>>(
              memReader, m_valueReader,
              this->m_edge[i].m_str.back());
    }
    else
    */
    {
      if (m_edgeInfo[i].m_isLeaf)
        return make_unique<LeafIterator0<TValueReader>>(m_reader.SubReader(offset, size),
                                                        m_valueReader);
      else
        return make_unique<Iterator0<TReader, TValueReader>>(
            m_reader.SubReader(offset, size), m_valueReader, this->m_edge[i].m_str.back());
    }
  }

private:
  void ParseNode(TrieChar baseChar)
  {
    ReaderSource<TReader> src(m_reader);

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
      typename Iterator<ValueType>::Edge & e = this->m_edge[i];

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

  TReader m_reader;
  TValueReader m_valueReader;
};

// Returns iterator to the root of the trie.
template <class TReader, class TValueReader>
unique_ptr<Iterator<typename TValueReader::ValueType>> ReadTrie(
    TReader const & reader, TValueReader valueReader = TValueReader())
{
  return make_unique<Iterator0<TReader, TValueReader>>(reader, valueReader, DEFAULT_CHAR);
}

}  // namespace trie
