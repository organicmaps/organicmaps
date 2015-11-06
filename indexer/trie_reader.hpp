#pragma once
#include "indexer/trie.hpp"

#include "coding/reader.hpp"
#include "coding/varint.hpp"

#include "base/assert.hpp"
#include "base/bits.hpp"
#include "base/macros.hpp"

namespace trie
{
template <class TValueList, typename TSerializer>
class LeafIterator0 : public Iterator<TValueList>
{
public:
  using TValue = typename TValueList::TValue;
  using Iterator<TValueList>::m_valueList;

  template <class TReader>
  LeafIterator0(TReader const & reader, TSerializer const & serializer)
  {
    ReaderSource<TReader> src(reader);
    m_valueList.Deserialize(src, serializer);
    ASSERT_EQUAL(src.Size(), 0, ());
  }

  // trie::Iterator overrides:
  unique_ptr<Iterator<TValueList>> Clone() const override
  {
    return make_unique<LeafIterator0<TValueList, TSerializer>>(*this);
  }

  unique_ptr<Iterator<TValueList>> GoToEdge(size_t /* i */) const override
  {
    ASSERT(false, ());
    return nullptr;
  }
};

template <typename TReader, typename TValueList, typename TSerializer>
class Iterator0 : public Iterator<TValueList>
{
public:
  using TValue = typename TValueList::TValue;
  using Iterator<TValueList>::m_valueList;
  using Iterator<TValueList>::m_edge;

  Iterator0(TReader const & reader, TrieChar baseChar, TSerializer const & serializer)
    : m_reader(reader), m_serializer(serializer)
  {
    ParseNode(baseChar);
  }

  // trie::Iterator overrides:
  unique_ptr<Iterator<TValueList>> Clone() const override
  {
    return make_unique<Iterator0<TReader, TValueList, TSerializer>>(*this);
  }

  unique_ptr<Iterator<TValueList>> GoToEdge(size_t i) const override
  {
    ASSERT_LESS(i, this->m_edge.size(), ());
    uint32_t const offset = m_edgeInfo[i].m_offset;
    uint32_t const size = m_edgeInfo[i + 1].m_offset - offset;

    if (m_edgeInfo[i].m_isLeaf)
    {
      return make_unique<LeafIterator0<TValueList, TSerializer>>(m_reader.SubReader(offset, size),
                                                                 m_serializer);
    }

    return make_unique<Iterator0<TReader, TValueList, TSerializer>>(
        m_reader.SubReader(offset, size), this->m_edge[i].m_label.back(), m_serializer);
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

    // [valueList]
    m_valueList.Deserialize(src, valueCount, m_serializer);

    // [childInfo] ... [childInfo]
    this->m_edge.resize(childCount);
    m_edgeInfo.resize(childCount + 1);
    m_edgeInfo[0].m_offset = 0;
    for (uint32_t i = 0; i < childCount; ++i)
    {
      typename Iterator<TValueList>::Edge & e = this->m_edge[i];

      // [1: header]: [1: isLeaf] [1: isShortEdge] [6: (edgeChar0 - baseChar) or min(edgeLen-1, 63)]
      uint8_t const header = ReadPrimitiveFromSource<uint8_t>(src);
      m_edgeInfo[i].m_isLeaf = ((header & 128) != 0);
      if (header & 64)
        e.m_label.push_back(baseChar + bits::ZigZagDecode(header & 63U));
      else
      {
        // [vu edgeLen-1]: if edgeLen-1 in header == 63
        uint32_t edgeLen = (header & 63);
        if (edgeLen == 63)
          edgeLen = ReadVarUint<uint32_t>(src);
        edgeLen += 1;

        // [vi edgeChar0 - baseChar] [vi edgeChar1 - edgeChar0] ... [vi edgeCharN - edgeCharN-1]
        e.m_label.reserve(edgeLen);
        for (uint32_t i = 0; i < edgeLen; ++i)
          e.m_label.push_back(baseChar += ReadVarInt<int32_t>(src));
      }

      // [child size]: if the child is not the last one
      m_edgeInfo[i + 1].m_offset = m_edgeInfo[i].m_offset;
      if (i != childCount - 1)
        m_edgeInfo[i + 1].m_offset += ReadVarUint<uint32_t>(src);

      baseChar = e.m_label[0];
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
  TSerializer m_serializer;
};

// Returns iterator to the root of the trie.
template <class TReader, class TValueList, class TSerializer>
unique_ptr<Iterator<TValueList>> ReadTrie(TReader const & reader, TSerializer const & serializer)
{
  return make_unique<Iterator0<TReader, TValueList, TSerializer>>(reader, DEFAULT_CHAR, serializer);
}

}  // namespace trie
