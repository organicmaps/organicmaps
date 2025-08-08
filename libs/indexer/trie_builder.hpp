#pragma once
#include "indexer/trie.hpp"

#include "coding/byte_stream.hpp"
#include "coding/varint.hpp"

#include "base/buffer_vector.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

// Trie format:
// [1: header]
// [node] ... [node]

// Nodes are written in post-order (first child, last child, parent). Contents of nodes is writern
// reversed. The resulting file should be reverese before use! Then its contents will appear in
// pre-order alphabetically reversed (parent, last child, first child).

// Leaf node format:
// [valueList]

// Internal node format:
// [1: header]: [2: min(valueCount, 3)] [6: min(childCount, 63)]
// [vu valueCount]: if valueCount in header == 3
// [vu childCount]: if childCount in header == 63
// [valueList]
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
// [child size]: if the child is not the last one when reading

namespace trie
{
template <typename Sink, typename ChildIter, typename ValueList, typename Serializer>
void WriteNode(Sink & sink, Serializer const & serializer, TrieChar baseChar, ValueList const & valueList,
               ChildIter const begChild, ChildIter const endChild, bool isRoot = false)
{
  uint32_t const valueCount = base::asserted_cast<uint32_t>(valueList.Size());
  if (begChild == endChild && !isRoot)
  {
// Leaf node.
#ifdef DEBUG
    auto posBefore = sink.Pos();
#endif

    valueList.Serialize(sink, serializer);

#ifdef DEBUG
    if (valueCount == 0)
      ASSERT_EQUAL(sink.Pos(), posBefore, ("Empty valueList must produce an empty serialization."));
#endif

    return;
  }
  uint32_t const childCount = base::asserted_cast<uint32_t>(endChild - begChild);
  uint8_t const header = static_cast<uint32_t>((std::min(valueCount, 3U) << 6) + std::min(childCount, 63U));
  sink.Write(&header, 1);
  if (valueCount >= 3)
    WriteVarUint(sink, valueCount);
  if (childCount >= 63)
    WriteVarUint(sink, childCount);
  valueList.Serialize(sink, serializer);
  for (ChildIter it = begChild; it != endChild;)
  {
    uint8_t header = (it->IsLeaf() ? 128 : 0);
    TrieChar const * const edge = it->GetEdge();
    uint32_t const edgeSize = base::asserted_cast<uint32_t>(it->GetEdgeSize());
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

    uint32_t const childSize = it->Size();
    if (++it != endChild)
      WriteVarUint(sink, childSize);
  }
}

struct ChildInfo
{
  ChildInfo(bool isLeaf, uint32_t size, TrieChar c) : m_isLeaf(isLeaf), m_size(size), m_edge(1, c) {}

  uint32_t Size() const { return m_size; }
  bool IsLeaf() const { return m_isLeaf; }
  TrieChar const * GetEdge() const { return m_edge.data(); }
  size_t GetEdgeSize() const { return m_edge.size(); }

  bool m_isLeaf;
  uint32_t m_size;
  std::vector<TrieChar> m_edge;
};

template <typename ValueList>
struct NodeInfo
{
  NodeInfo() = default;
  NodeInfo(uint64_t pos, TrieChar trieChar) : m_begPos(pos), m_char(trieChar) {}

  // It is finalized in the sense that no more appends are possible
  // so it is a fine moment to initialize the underlying ValueList.
  void FinalizeValueList()
  {
    m_valueList.Init(m_temporaryValueList);
    m_mayAppend = false;
  }

  uint64_t m_begPos = 0;
  TrieChar m_char = 0;
  std::vector<ChildInfo> m_children;

  // This is ugly but will do until we rename ValueList.
  // Here is the rationale. ValueList<> is the entity that
  // we store in the nodes of the search trie. It can be read
  // or written via its methods but not directly as was assumed
  // in a previous version of this code. That version provided
  // serialization methods for ValueList but the deserialization
  // was ad hoc.
  // Because of the possibility of serialized ValueLists to represent
  // something completely different from an array of FeatureIds
  // (a compressed bit vector, for instance) and because of the
  // need to update a node's ValueList until the node is finalized
  // this vector is needed here. It is better to leave it here
  // than to expose it in ValueList.
  std::vector<typename ValueList::Value> m_temporaryValueList;
  ValueList m_valueList = {};
  bool m_mayAppend = true;
};

template <typename Sink, typename ValueList, typename Serializer>
void WriteNodeReverse(Sink & sink, Serializer const & serializer, TrieChar baseChar, NodeInfo<ValueList> & node,
                      bool isRoot = false)
{
  using OutStorage = buffer_vector<uint8_t, 64>;
  OutStorage out;
  PushBackByteSink<OutStorage> outSink(out);
  node.FinalizeValueList();
  WriteNode(outSink, serializer, baseChar, node.m_valueList, node.m_children.rbegin(), node.m_children.rend(), isRoot);
  std::reverse(out.begin(), out.end());
  sink.Write(out.data(), out.size());
}

template <typename Sink, typename Nodes, typename Serializer>
void PopNodes(Sink & sink, Serializer const & serializer, Nodes & nodes, size_t nodesToPop)
{
  ASSERT_GREATER(nodes.size(), nodesToPop, ());
  for (; nodesToPop > 0; --nodesToPop)
  {
    auto & node = nodes.back();
    auto & prevNode = nodes[nodes.size() - 2];

    if (node.m_temporaryValueList.empty() && node.m_children.size() <= 1)
    {
      ASSERT_EQUAL(node.m_children.size(), 1, ());
      auto & child = node.m_children[0];
      prevNode.m_children.emplace_back(child.m_isLeaf, child.m_size, node.m_char);
      auto & prevChild = prevNode.m_children.back();
      prevChild.m_edge.insert(prevChild.m_edge.end(), child.m_edge.begin(), child.m_edge.end());
    }
    else
    {
      WriteNodeReverse(sink, serializer, node.m_char, node);
      prevNode.m_children.emplace_back(node.m_children.empty(), static_cast<uint32_t>(sink.Pos() - node.m_begPos),
                                       node.m_char);
    }

    nodes.pop_back();
  }
}

template <typename NodeInfo, typename Value>
void AppendValue(NodeInfo & node, Value const & value)
{
  // External-memory trie adds <string, value> pairs in a sorted
  // order so the values are supposed to be accumulated in the
  // sorted order and we can avoid sorting them before doing
  // further operations such as ValueList construction.
  ASSERT(node.m_temporaryValueList.empty() || node.m_temporaryValueList.back() <= value,
         (node.m_temporaryValueList.size()));
  if (!node.m_temporaryValueList.empty() && node.m_temporaryValueList.back() == value)
    return;
  if (node.m_mayAppend)
    node.m_temporaryValueList.push_back(value);
  else
    LOG(LERROR, ("Cannot append to a finalized value list."));
}

template <typename Sink, typename Key, typename ValueList, typename Serializer>
void Build(Sink & sink, Serializer const & serializer,
           std::vector<std::pair<Key, typename ValueList::Value>> const & data)
{
  using Value = typename ValueList::Value;
  using NodeInfo = NodeInfo<ValueList>;

  std::vector<NodeInfo> nodes;
  nodes.emplace_back(sink.Pos(), kDefaultChar);

  Key prevKey;
  std::pair<Key, Value> prevE;  // e for "element".

  for (auto it = data.begin(); it != data.end(); ++it)
  {
    auto e = *it;
    if (it != data.begin() && e == prevE)
      continue;

    auto const & key = e.first;
    CHECK(!(key < prevKey), (key, prevKey));
    size_t nCommon = 0;
    while (nCommon < std::min(key.size(), prevKey.size()) && prevKey[nCommon] == key[nCommon])
      ++nCommon;

    // Root is also a common node.
    PopNodes(sink, serializer, nodes, nodes.size() - nCommon - 1);
    uint64_t const pos = sink.Pos();
    for (size_t i = nCommon; i < key.size(); ++i)
      nodes.emplace_back(pos, key[i]);
    AppendValue(nodes.back(), e.second);

    prevKey = key;
    std::swap(e, prevE);
  }

  // Pop all the nodes from the stack.
  PopNodes(sink, serializer, nodes, nodes.size() - 1);

  // Write the root.
  WriteNodeReverse(sink, serializer, kDefaultChar /* baseChar */, nodes.back(), true /* isRoot */);
}
}  // namespace trie
