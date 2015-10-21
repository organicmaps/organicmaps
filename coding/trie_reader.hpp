#pragma once
#include "coding/trie.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"

#include "indexer/coding_params.hpp"
#include "indexer/string_file_values.hpp"

#include "base/assert.hpp"
#include "base/bits.hpp"
#include "base/macros.hpp"

namespace trie
{
template <class TValueList>
class LeafIterator0 : public Iterator<TValueList>
{
public:
  using Iterator<TValueList>::m_valueList;

  template <class TReader>
  LeafIterator0(TReader const & reader, serial::CodingParams const & codingParams)
  {
    ReaderSource<TReader> src(reader);
    uint32_t valueCount = ReadVarUint<uint32_t>(src);
    m_valueList.SetCodingParams(codingParams);
    m_valueList.Deserialize(src, valueCount);
    // todo(@mpimenov) There used to be an assert here
    // that src is completely exhausted by this time.
  }

  // trie::Iterator overrides:
  unique_ptr<Iterator<TValueList>> Clone() const override
  {
    return make_unique<LeafIterator0<TValueList>>(*this);
  }

  unique_ptr<Iterator<TValueList>> GoToEdge(size_t i) const override
  {
    ASSERT(false, (i));
    UNUSED_VALUE(i);
    return nullptr;
  }
};

template <class TReader, class TValueList>
class Iterator0 : public Iterator<TValueList>
{
public:
  using Iterator<TValueList>::m_valueList;
  using Iterator<TValueList>::m_edge;

  Iterator0(TReader const & reader, TrieChar baseChar, serial::CodingParams const & codingParams)
    : m_reader(reader), m_codingParams(codingParams)
  {
    m_valueList.SetCodingParams(m_codingParams);
    ParseNode(baseChar);
  }

  // trie::Iterator overrides:
  unique_ptr<Iterator<TValueList>> Clone() const override
  {
    return make_unique<Iterator0<TReader, TValueList>>(*this);
  }

  unique_ptr<Iterator<TValueList>> GoToEdge(size_t i) const override
  {
    ASSERT_LESS(i, this->m_edge.size(), ());
    uint32_t const offset = m_edgeInfo[i].m_offset;
    uint32_t const size = m_edgeInfo[i+1].m_offset - offset;

    if (m_edgeInfo[i].m_isLeaf)
    {
      return make_unique<LeafIterator0<TValueList>>(m_reader.SubReader(offset, size),
                                                    m_codingParams);
    }

    return make_unique<Iterator0<TReader, TValueList>>(
        m_reader.SubReader(offset, size), this->m_edge[i].m_str.back(), m_codingParams);
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
    m_valueList.Deserialize(src, valueCount);

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
  serial::CodingParams m_codingParams;
};

// Returns iterator to the root of the trie.
template <class TReader, class TValueList>
unique_ptr<Iterator<TValueList>> ReadTrie(TReader const & reader,
                                          serial::CodingParams const & codingParams)
{
  return make_unique<Iterator0<TReader, TValueList>>(reader, DEFAULT_CHAR, codingParams);
}

}  // namespace trie
