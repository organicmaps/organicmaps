#pragma once

#include "coding/bit_streams.hpp"
#include "coding/byte_stream.hpp"
#include "coding/huffman.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/writer.hpp"

#include "base/buffer_vector.hpp"
#include "base/checked_cast.hpp"
#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <functional>
#include <vector>

// Trie format:
//   -- Serialized Huffman encoding.
//   -- Topology of the trie built on Huffman-encoded input strings [2 bits per node, level-order representation].
//   -- List of pairs (node id, offset). One pair per final node (i.e. a node where a string ends).
//      The lists of node ids and offsets are both non-decreasing and are delta-encoded with
//      varuints.
//   -- Values of final nodes in level-order. The values for final node |id| start at offset |offset|
//      if there is a pair (id, offset) in the list above.

namespace trie
{
// Node is a temporary struct that is used to store the trie in memory
// before it is converted to a succinct representation and put to disk.
// It is rather verbose but hopefully is small enough to fit in memory.
template <class TValueList>
struct Node
{
  // The left child is reached by 0, the right by 1.
  Node *l, *r;

  // A node is final if a key string ends in it.
  // In particular, all non-final nodes' valueLists are empty
  // and it is expected that a final node's valueList is non-empty.
  bool m_isFinal;

  // m_valueLists stores all the additional information that is related
  // to the key string which leads to this node.
  TValueList m_valueList;

  Node() : l(nullptr), r(nullptr), m_isFinal(false) {}
};

template <class TNode, class TReader>
TNode * AddToTrie(TNode * root, TReader & bitEncoding, uint32_t numBits)
{
  ReaderSource<TReader> src(bitEncoding);
  // String size.
  ReadVarUint<uint32_t, ReaderSource<TReader>>(src);

  BitReader<ReaderSource<TReader>> bitReader(src);
  auto cur = root;
  for (uint32_t i = 0; i < numBits; ++i)
  {
    auto nxt = bitReader.Read(1) == 0 ? &cur->l : &cur->r;
    if (!*nxt)
      *nxt = new TNode();
    cur = *nxt;
  }
  return cur;
}

template <class TNode>
void DeleteTrie(TNode * root)
{
  if (!root)
    return;
  DeleteTrie(root->l);
  DeleteTrie(root->r);
  delete root;
}

template <typename TNode>
void WriteInLevelOrder(TNode * root, std::vector<TNode *> & levelOrder)
{
  ASSERT(root, ());
  levelOrder.push_back(root);
  size_t i = 0;
  while (i < levelOrder.size())
  {
    TNode * cur = levelOrder[i++];
    if (cur->l)
      levelOrder.push_back(cur->l);
    if (cur->r)
      levelOrder.push_back(cur->r);
  }
}

template <typename TWriter, typename TIter, typename TValueList>
void BuildSuccinctTrie(TWriter & writer, TIter const beg, TIter const end)
{
  using TrieChar = char32_t;
  using TTrieString = buffer_vector<TrieChar, 32>;
  using TNode = Node<TValueList>;
  using TEntry = typename TIter::value_type;

  TNode * root = new TNode();
  SCOPE_GUARD(cleanup, std::bind(&DeleteTrie<TNode>, root));
  TTrieString prevKey;
  TEntry prevEntry;

  std::vector<TEntry> entries;
  std::vector<strings::UniString> entryStrings;
  for (TIter it = beg; it != end; ++it)
  {
    TEntry entry = *it;
    if (it != beg && entry == prevEntry)
      continue;
    TrieChar const * const keyData = entry.GetKeyData();
    TTrieString key(keyData, keyData + entry.GetKeySize());

    CHECK_GREATER_OR_EQUAL(key, prevKey, (key, prevKey));
    entries.push_back(entry);
    entryStrings.push_back(strings::UniString(keyData, keyData + entry.GetKeySize()));
    prevKey.swap(key);
    prevEntry.Swap(entry);
  }

  coding::HuffmanCoder huffman;
  huffman.Init(entryStrings);
  huffman.WriteEncoding(writer);

  for (size_t i = 0; i < entries.size(); ++i)
  {
    TEntry const & entry = entries[i];
    auto const & key = entryStrings[i];

    std::vector<uint8_t> buf;
    MemWriter<std::vector<uint8_t>> memWriter(buf);
    uint32_t numBits = huffman.EncodeAndWrite(memWriter, key);

    MemReader bitEncoding(&buf[0], buf.size());

    TNode * cur = AddToTrie(root, bitEncoding, numBits);
    cur->m_isFinal = true;
    cur->m_valueList.Append(entry.GetValue());
  }

  std::vector<TNode *> levelOrder;
  WriteInLevelOrder(root, levelOrder);

  {
    // Trie topology.
    BitWriter<TWriter> bitWriter(writer);
    WriteVarUint(writer, levelOrder.size());
    for (size_t i = 0; i < levelOrder.size(); ++i)
    {
      auto const & cur = levelOrder[i];
      bitWriter.Write(cur->l ? 1 : 0, 1 /* numBits */);
      bitWriter.Write(cur->r ? 1 : 0, 1 /* numBits */);
    }
  }

  std::vector<uint32_t> finalNodeIds;
  std::vector<uint32_t> offsetTable;
  std::vector<uint8_t> valueBuf;
  MemWriter<std::vector<uint8_t>> valueWriter(valueBuf);
  for (size_t i = 0; i < levelOrder.size(); ++i)
  {
    TNode const * node = levelOrder[i];
    if (!node->m_isFinal)
      continue;
    finalNodeIds.push_back(static_cast<uint32_t>(i));
    offsetTable.push_back(base::asserted_cast<uint32_t>(valueWriter.Pos()));
    node->m_valueList.Dump(valueWriter);
  }

  WriteVarUint(writer, finalNodeIds.size());
  for (size_t i = 0; i < finalNodeIds.size(); ++i)
  {
    if (i == 0)
    {
      WriteVarUint(writer, finalNodeIds[i]);
      WriteVarUint(writer, offsetTable[i]);
    }
    else
    {
      WriteVarUint(writer, finalNodeIds[i] - finalNodeIds[i - 1]);
      WriteVarUint(writer, offsetTable[i] - offsetTable[i - 1]);
    }
  }

  writer.Write(valueBuf.data(), valueBuf.size());

  // todo(@pimenov):
  // 1. Investigate the possibility of path compression (short edges + lcp table).
  // 2. It is highly probable that valueList will be only a list of feature ids in our
  //    future implementations of the search.
  //    We can then dispose of offsetTable and store finalNodeIds as another bit vector.
  //    Alternatively (and probably better), we can append a special symbol to all those
  //    strings that have non-empty value lists. We can then get the leaf number
  //    of a final node by doing rank0.
}

}  // namespace trie
