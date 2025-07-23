#pragma once

#include "indexer/trie.hpp"

#include "coding/reader.hpp"
#include "coding/varint.hpp"

#include "base/assert.hpp"
#include "base/bits.hpp"
#include "base/macros.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace trie
{
template <class ValueList, typename Serializer>
class LeafIterator0 final : public Iterator<ValueList>
{
public:
  using Value = typename ValueList::Value;
  using Iterator<ValueList>::m_values;

  template <class Reader>
  LeafIterator0(Reader const & reader, Serializer const & serializer)
  {
    ReaderSource<Reader> source(reader);
    m_values.Deserialize(source, serializer);
    ASSERT_EQUAL(source.Size(), 0, ());
  }

  ~LeafIterator0() override = default;

  // trie::Iterator overrides:
  std::unique_ptr<Iterator<ValueList>> Clone() const override
  {
    return std::make_unique<LeafIterator0<ValueList, Serializer>>(*this);
  }

  std::unique_ptr<Iterator<ValueList>> GoToEdge(size_t /* i */) const override
  {
    ASSERT(false, ());
    return nullptr;
  }
};

template <typename Reader, typename ValueList, typename Serializer>
class Iterator0 final : public Iterator<ValueList>
{
public:
  using Value = typename ValueList::Value;
  using Iterator<ValueList>::m_values;
  using Iterator<ValueList>::m_edges;

  Iterator0(Reader const & reader, TrieChar baseChar, Serializer const & serializer)
    : m_reader(reader)
    , m_serializer(serializer)
  {
    ParseNode(baseChar);
  }

  ~Iterator0() override = default;

  // trie::Iterator overrides:
  std::unique_ptr<Iterator<ValueList>> Clone() const override
  {
    return std::make_unique<Iterator0<Reader, ValueList, Serializer>>(*this);
  }

  std::unique_ptr<Iterator<ValueList>> GoToEdge(size_t i) const override
  {
    ASSERT_LESS(i, this->m_edges.size(), ());
    uint32_t const offset = m_edgeInfo[i].m_offset;
    uint32_t const size = m_edgeInfo[i + 1].m_offset - offset;

    if (m_edgeInfo[i].m_isLeaf)
      return std::make_unique<LeafIterator0<ValueList, Serializer>>(m_reader.SubReader(offset, size), m_serializer);

    return std::make_unique<Iterator0<Reader, ValueList, Serializer>>(m_reader.SubReader(offset, size),
                                                                      this->m_edges[i].m_label.back(), m_serializer);
  }

private:
  void ParseNode(TrieChar baseChar)
  {
    ReaderSource<Reader> source(m_reader);

    // [1: header]: [2: min(valueCount, 3)] [6: min(childCount, 63)]
    uint8_t const header = ReadPrimitiveFromSource<uint8_t>(source);
    uint32_t valueCount = (header >> 6);
    uint32_t childCount = (header & 63);

    // [vu valueCount]: if valueCount in header == 3
    if (valueCount == 3)
      valueCount = ReadVarUint<uint32_t>(source);

    // [vu childCount]: if childCount in header == 63
    if (childCount == 63)
      childCount = ReadVarUint<uint32_t>(source);

    // [valueList]
    m_values.Deserialize(source, valueCount, m_serializer);

    // [childInfo] ... [childInfo]
    this->m_edges.resize(childCount);
    m_edgeInfo.resize(childCount + 1);
    m_edgeInfo[0].m_offset = 0;
    for (uint32_t i = 0; i < childCount; ++i)
    {
      auto & e = this->m_edges[i];

      // [1: header]: [1: isLeaf] [1: isShortEdge] [6: (edgeChar0 - baseChar) or min(edgeLen-1, 63)]
      uint8_t const header = ReadPrimitiveFromSource<uint8_t>(source);
      m_edgeInfo[i].m_isLeaf = ((header & 128) != 0);
      if (header & 64)
        e.m_label.push_back(baseChar + bits::ZigZagDecode(header & 63U));
      else
      {
        // [vu edgeLen-1]: if edgeLen-1 in header == 63
        uint32_t edgeLen = (header & 63);
        if (edgeLen == 63)
          edgeLen = ReadVarUint<uint32_t>(source);
        edgeLen += 1;

        // [vi edgeChar0 - baseChar] [vi edgeChar1 - edgeChar0] ... [vi edgeCharN - edgeCharN-1]
        e.m_label.reserve(edgeLen);
        for (uint32_t i = 0; i < edgeLen; ++i)
          e.m_label.push_back(baseChar += ReadVarInt<int32_t>(source));
      }

      // [child size]: if the child is not the last one
      m_edgeInfo[i + 1].m_offset = m_edgeInfo[i].m_offset;
      if (i != childCount - 1)
        m_edgeInfo[i + 1].m_offset += ReadVarUint<uint32_t>(source);

      baseChar = e.m_label[0];
    }

    uint32_t const currentOffset = static_cast<uint32_t>(source.Pos());
    for (size_t i = 0; i < m_edgeInfo.size(); ++i)
      m_edgeInfo[i].m_offset += currentOffset;
    m_edgeInfo.back().m_offset = static_cast<uint32_t>(m_reader.Size());
  }

  struct EdgeInfo
  {
    uint32_t m_offset = 0;
    bool m_isLeaf = false;
  };

  buffer_vector<EdgeInfo, 9> m_edgeInfo;

  Reader m_reader;
  Serializer m_serializer;
};

// Returns iterator to the root of the trie.
template <class Reader, class ValueList, class Serializer>
std::unique_ptr<Iterator<ValueList>> ReadTrie(Reader const & reader, Serializer const & serializer)
{
  return std::make_unique<Iterator0<Reader, ValueList, Serializer>>(reader, kDefaultChar, serializer);
}
}  // namespace trie
