#pragma once

#include "coding/byte_stream.hpp"
#include "coding/varint.hpp"

#include "base/buffer_vector.hpp"

#include "std/algorithm.hpp"

// Trie format:
// [1: header]
// [node] ... [node]

// Nodes are written in post-order (first child, last child, parent). Contents of nodes is writern
// reversed. The resulting file should be reverese before use! Then its contents will appear in
// pre-order alphabetically reversed (parent, last child, first child).

// Leaf node format:
// [value] ... [value]

// Internal node format:
// [1: header]: [2: min(valueCount, 3)] [6: min(childCount, 63)]
// [vu valueCount]: if valueCount in header == 3
// [vu childCount]: if childCount in header == 63
// [value] ... [value]
// [childInfo] ... [childInfo]

// Child info format:
// Every char of the edge is encoded as varint difference from the previous char. First char is
// encoded as varint difference from the base char, which is the last char of the current prefix.
//
// [1: header]: [1: isLeaf] [1: isShortEdge] [6: (edgeChar0 - baseChar) or min(edgeLen-1, 63)]
// [vu edgeLen-1]: if edgeLen-1 in header == 63
// [vi edgeChar0 - baseChar]
// [vi edgeChar1 - edgeChar0]
// ...
// [vi edgeCharN - edgeCharN-1]
// [edge value]
// [child size]: if the child is not the last one when reading

namespace trie
{
template <typename TSink, typename TChildIter, typename TValueList>
void WriteNode(TSink & sink, TrieChar baseChar, TValueList const & valueList,
               TChildIter const begChild, TChildIter const endChild, bool isRoot = false)
{
  if (begChild == endChild && !isRoot)
  {
    // Leaf node.
    valueList.Dump(sink);
    return;
  }
  uint32_t const childCount = endChild - begChild;
  uint32_t const valueCount = valueList.size();
  uint8_t const header = static_cast<uint32_t>((min(valueCount, 3U) << 6) + min(childCount, 63U));
  sink.Write(&header, 1);
  if (valueCount >= 3)
    WriteVarUint(sink, valueCount);
  if (childCount >= 63)
    WriteVarUint(sink, childCount);
  valueList.Dump(sink);
  for (TChildIter it = begChild; it != endChild; /*++it*/)
  {
    uint8_t header = (it->IsLeaf() ? 128 : 0);
    TrieChar const * const edge = it->GetEdge();
    uint32_t const edgeSize = it->GetEdgeSize();
    CHECK_NOT_EQUAL(edgeSize, 0, ());
    CHECK_LESS(edgeSize, 100000, ());
    uint32_t const diff0 = bits::ZigZagEncode(int32_t(edge[0] - baseChar));
    if (edgeSize == 1 && (diff0 & ~63U) == 0)
    {
      header |= 64;
      header |= diff0;
      WriteToSink(sink, header);
    }
    else
    {
      if (edgeSize - 1 < 63)
      {
        header |= edgeSize - 1;
        WriteToSink(sink, header);
      }
      else
      {
        header |= 63;
        WriteToSink(sink, header);
        WriteVarUint(sink, edgeSize - 1);
      }
      for (uint32_t i = 0; i < edgeSize; ++i)
      {
        WriteVarInt(sink, int32_t(edge[i] - baseChar));
        baseChar = edge[i];
      }
    }
    baseChar = edge[0];
    sink.Write(it->GetEdgeValue(), it->GetEdgeValueSize());

    uint32_t const childSize = it->Size();
    if (++it != endChild)
      WriteVarUint(sink, childSize);
  }
}

struct ChildInfo
{
  bool m_isLeaf;
  uint32_t m_size;
  buffer_vector<TrieChar, 8> m_edge;
  using TEdgeValueStorage = buffer_vector<uint8_t, 8>;
  TEdgeValueStorage m_edgeValue;

  ChildInfo(bool isLeaf, uint32_t size, TrieChar c) : m_isLeaf(isLeaf), m_size(size), m_edge(1, c)
  {
  }

  uint32_t Size() const { return m_size; }
  bool IsLeaf() const { return m_isLeaf; }
  TrieChar const * GetEdge() const { return m_edge.data(); }
  uint32_t GetEdgeSize() const { return m_edge.size(); }
  void const * GetEdgeValue() const { return m_edgeValue.data(); }
  uint32_t GetEdgeValueSize() const { return m_edgeValue.size(); }
};

template <class TEdgeBuilder, class TValueList>
struct NodeInfo
{
  uint64_t m_begPos;
  TrieChar m_char;
  vector<ChildInfo> m_children;
  TValueList m_valueList;
  TEdgeBuilder m_edgeBuilder;

  NodeInfo() : m_begPos(0), m_char(0) {}
  NodeInfo(uint64_t pos, TrieChar trieChar, TEdgeBuilder const & edgeBuilder)
    : m_begPos(pos), m_char(trieChar), m_edgeBuilder(edgeBuilder)
  {
  }
};

template <typename TSink, typename TEdgeBuilder, typename TValueList>
void WriteNodeReverse(TSink & sink, TrieChar baseChar,
                      NodeInfo<TEdgeBuilder, TValueList> const & node, bool isRoot = false)
{
  using TOutStorage = buffer_vector<uint8_t, 64>;
  TOutStorage out;
  PushBackByteSink<TOutStorage> outSink(out);
  WriteNode(outSink, baseChar, node.m_valueList, node.m_children.rbegin(), node.m_children.rend(),
            isRoot);
  reverse(out.begin(), out.end());
  sink.Write(out.data(), out.size());
}

template <typename TSink, class TNodes>
void PopNodes(TSink & sink, TNodes & nodes, int nodesToPop)
{
  using TNodeInfo = typename TNodes::value_type;
  ASSERT_GREATER(nodes.size(), nodesToPop, ());
  for (; nodesToPop > 0; --nodesToPop)
  {
    TNodeInfo & node = nodes.back();
    TNodeInfo & prevNode = nodes[nodes.size() - 2];

    if (node.m_valueList.empty() && node.m_children.size() <= 1)
    {
      ASSERT_EQUAL(node.m_children.size(), 1, ());
      ChildInfo & child = node.m_children[0];
      prevNode.m_children.push_back(ChildInfo(child.m_isLeaf, child.m_size, node.m_char));
      prevNode.m_children.back().m_edge.append(child.m_edge.begin(), child.m_edge.end());
    }
    else
    {
      WriteNodeReverse(sink, node.m_char, node);
      prevNode.m_children.push_back(ChildInfo(node.m_children.empty(),
                                              static_cast<uint32_t>(sink.Pos() - node.m_begPos),
                                              node.m_char));
    }

    prevNode.m_edgeBuilder.AddEdge(node.m_edgeBuilder);
    PushBackByteSink<ChildInfo::TEdgeValueStorage> sink(prevNode.m_children.back().m_edgeValue);
    node.m_edgeBuilder.StoreValue(sink);

    nodes.pop_back();
  }
}

struct EmptyEdgeBuilder
{
  using ValueType = unsigned char;

  void AddValue(void const *, uint32_t) {}
  void AddEdge(EmptyEdgeBuilder &) {}
  template <typename TSink>
  void StoreValue(TSink &) const
  {
  }
};

template <typename TMaxValueCalc>
struct MaxValueEdgeBuilder
{
  using ValueType = typename TMaxValueCalc::ValueType;

  TMaxValueCalc m_maxCalc;
  ValueType m_value;

  explicit MaxValueEdgeBuilder(TMaxValueCalc const & maxCalc = TMaxValueCalc())
    : m_maxCalc(maxCalc), m_value()
  {
  }

  MaxValueEdgeBuilder(MaxValueEdgeBuilder<TMaxValueCalc> const & edgeBuilder)
    : m_maxCalc(edgeBuilder.m_maxCalc), m_value(edgeBuilder.m_value)
  {
  }

  void AddValue(void const * p, uint32_t size)
  {
    ValueType value = m_maxCalc(p, size);
    if (m_value < value)
      m_value = value;
  }

  void AddEdge(MaxValueEdgeBuilder & edgeBuilder)
  {
    if (m_value < edgeBuilder.m_value)
      m_value = edgeBuilder.m_value;
  }

  template <typename TSink>
  void StoreValue(TSink & sink) const
  {
    sink.Write(&m_value, sizeof(m_value));
  }
};

template <typename TSink, typename TIter, typename TEdgeBuilder, typename TValueList>
void Build(TSink & sink, TIter const beg, TIter const end, TEdgeBuilder const & edgeBuilder)
{
  using TTrieString = buffer_vector<TrieChar, 32>;
  using TNodeInfo = NodeInfo<TEdgeBuilder, TValueList>;

  buffer_vector<TNodeInfo, 32> nodes;
  nodes.push_back(TNodeInfo(sink.Pos(), DEFAULT_CHAR, edgeBuilder));

  TTrieString prevKey;

  using TElement = typename TIter::value_type;
  TElement prevE;

  for (TIter it = beg; it != end; ++it)
  {
    TElement e = *it;
    if (it != beg && e == prevE)
      continue;

    TrieChar const * const pKeyData = e.GetKeyData();
    TTrieString key(pKeyData, pKeyData + e.GetKeySize());
    CHECK(!(key < prevKey), (key, prevKey));
    size_t nCommon = 0;
    while (nCommon < min(key.size(), prevKey.size()) && prevKey[nCommon] == key[nCommon])
      ++nCommon;

    PopNodes(sink, nodes, nodes.size() - nCommon - 1);  // Root is also a common node.

    uint64_t const pos = sink.Pos();
    for (size_t i = nCommon; i < key.size(); ++i)
      nodes.push_back(TNodeInfo(pos, key[i], edgeBuilder));
    nodes.back().m_valueList.Append(e.GetValue());

    nodes.back().m_edgeBuilder.AddValue(e.value_data(), e.value_size());

    prevKey.swap(key);
    prevE.Swap(e);
  }

  // Pop all the nodes from the stack.
  PopNodes(sink, nodes, nodes.size() - 1);

  // Write the root.
  WriteNodeReverse(sink, DEFAULT_CHAR /* baseChar */, nodes.back(), true /* isRoot */);
}

}  // namespace trie
