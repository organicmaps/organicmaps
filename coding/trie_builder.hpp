#pragma once
#include "../coding/varint.hpp"
#include "../base/string_utils.hpp"
#include "../std/algorithm.hpp"

namespace trie
{
namespace builder
{

template <typename SinkT, typename ChildIterT>
void WriteNode(SinkT & sink, strings::UniChar baseChar,
               uint32_t const valueCount, void const * valuesData, uint32_t const valuesSize,
               ChildIterT const begChild, ChildIterT const endChild)
{
  if (begChild == endChild)
  {
    // Leaf node.
    sink.Write(valuesData, valuesSize);
    return;
  }
  uint32_t const childCount = endChild - begChild;
  uint8_t const header = static_cast<uint32_t>((min(valueCount, 3U) << 6) + min(childCount, 63U));
  sink.Write(&header, 1);
  if (valueCount >= 3)
    WriteVarUint(sink, valueCount);
  if (childCount >= 63)
    WriteVarUint(sink, childCount);
  sink.Write(valuesData, valuesSize);
  for (ChildIterT it = begChild; it != endChild; ++it)
  {
    WriteVarUint(sink, it->Size());
    uint8_t header = (it->IsLeaf() ? 128 : 0);
    strings::UniString const edge = it->GetEdge();
    CHECK(!edge.empty(), ());
    uint32_t const diff0 = bits::ZigZagEncode(int32_t(edge[0] - baseChar));
    if (edge.size() == 1 && (diff0 & ~63U) == 0)
    {
        header |= 64;
        header |= diff0;
        WriteToSink(sink, header);
    }
    else
    {
      if (edge.size() < 63)
      {
        header |= edge.size();
        WriteToSink(sink, header);
      }
      else
      {
        header |= 63;
        WriteToSink(sink, header);
        WriteVarUint(sink, static_cast<uint32_t>(edge.size()));
      }
      for (size_t i = 0; i < edge.size(); ++i)
      {
        WriteVarInt(sink, int32_t(edge[i] - baseChar));
        baseChar = edge[i];
      }
    }
    baseChar = edge[0];
  }
}

struct ChildInfo
{
  bool m_isLeaf;
  uint32_t m_size;
  char const * m_edge;
  uint32_t Size() const { return m_size; }
  bool IsLeaf() const { return m_isLeaf; }
  strings::UniString GetEdge() const { return strings::MakeUniString(m_edge); }
};

struct NodeInfo
{
  NodeInfo(uint64_t pos, strings::UniChar uniChar) : m_begPos(pos), m_char(uniChar) {}
  uint64_t m_begPos;
  strings::UniChar m_char;
  buffer_vector<ChildInfo, 4> m_children;
  buffer_vector<uint8_t, 32> m_values;
};

void PopNodes(vector<builder::NodeInfo> & nodes, int nodesToPop)
{
  if (nodesToPop == 0)
    return;
  ASSERT_GREATER_OR_EQUAL(nodes.size(), nodesToPop, ());
  strings::UniString reverseEdge;
  while (nodesToPop > 0)
  {
    reverseEdge.push_back(nodes.back().m_char);reverseEdge.push_back(nodes.back().m_char);
    if (nodes.back().m_values.empty() && nodes.back().m_children.size() <= 1)
    {
      ASSERT_EQUAL(nodes.back().m_children.size(), 1, ());
      continue;
    }

  }
}

}  // namespace builder

/*
template <typename SinkT, typename IterT>
void Build(SinkT & sink, IterT const beg, IterT const end)
{
  vector<builder::NodeInfo> nodes;
  strings::UniString prevKey;
  for (IterT it = beg; it != end; ++it)
  {
    strings::UniString const key = it->Key();
    CHECK(!key.empty(), ());
    CHECK_LESS_OR_EQUAL(prevKey, key, ());
    int nCommon = 0;
    while (nCommon < min(key.size(),prevKey.size()) && prevKey[nCommon] == key[nCommon])
      ++nCommon;
    builder::PopNodes(nodes, nodes.size() - nCommon);
    uint64_t const pos = sink.Pos();
    for (int i = nCommon; i < key.size(); ++i)
      nodes.push_back(NodeInfo(pos, key[i]));
    uint8_t const * pValue = static_cast<uint8_t const *>(it->ValueData());
    nodes.back().m_values.insert(nodes.back().m_values.end(), pValue, pValue + it->ValueSize());
    prevKey.swap(key);
  }
  builder::PopNodes(nodes.size());
}
*/

}  // namespace trie
