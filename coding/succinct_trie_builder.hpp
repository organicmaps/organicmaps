#pragma once

#include "coding/bit_streams.hpp"
#include "coding/byte_stream.hpp"
#include "coding/huffman.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/writer.hpp"

#include "base/buffer_vector.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"

namespace trie
{
// Node is a temporary struct that is used to store the trie in memory
// before it is converted to a succinct representation and put to disk.
// It is rather verbose but hopefully is small enough to fit in memory.
template <class TEdgeBuilder, class TValueList>
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

  // m_edgeBuilder is obsolete and is here only for backward-compatibility
  // with an older implementation of the trie.
  // todo(@pimenov): Remove it.
  TEdgeBuilder m_edgeBuilder;

  Node() : l(nullptr), r(nullptr), m_isFinal(false) {}
};

template <class TNode, class TReader>
TNode * AddToTrie(TNode * root, TReader bitEncoding, uint32_t numBits)
{
  ReaderSource<TReader> src(bitEncoding);
  // String size.
  ReadVarUint<uint32_t, ReaderSource<TReader>>(src);

  BitReader<ReaderSource<TReader>> bitReader(src);
  auto cur = root;
  for (uint32_t i = 0; i < numBits; ++i)
  {
    uint8_t bit = bitReader.Read(1);
    if (bit == 0)
    {
      if (!cur->l)
        cur->l = new TNode();
      cur = cur->l;
    }
    else
    {
      if (!cur->r)
        cur->r = new TNode();
      cur = cur->r;
    }
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
vector<TNode *> WriteInLevelOrder(TNode * root)
{
  vector<TNode *> q;
  q.push_back(root);
  size_t qt = 0;
  while (qt < q.size())
  {
    TNode * cur = q[qt++];
    if (cur->l)
      q.push_back(cur->l);
    if (cur->r)
      q.push_back(cur->r);
  }
  return q;
}

template <typename TWriter, typename TIter, typename TEdgeBuilder, typename TValueList>
void BuildSuccinctTrie(TWriter & writer, TIter const beg, TIter const end,
                       TEdgeBuilder const & edgeBuilder)
{
  using TrieChar = uint32_t;
  using TTrieString = buffer_vector<TrieChar, 32>;
  using TNode = Node<TEdgeBuilder, TValueList>;
  using TEntry = typename TIter::value_type;

  TNode * root = new TNode();
  TTrieString prevKey;
  TEntry prevE;

  vector<TEntry> entries;
  vector<strings::UniString> entryStrings;
  for (TIter it = beg; it != end; ++it)
  {
    TEntry e = *it;
    if (it != beg && e == prevE)
      continue;
    TrieChar const * const keyData = e.GetKeyData();
    TTrieString key(keyData, keyData + e.GetKeySize());
    CHECK(!(key < prevKey), (key, prevKey));
    entries.push_back(e);
    entryStrings.push_back(strings::UniString(keyData, keyData + e.GetKeySize()));
    prevKey.swap(key);
    prevE.Swap(e);
  }

  coding::HuffmanCoder huffman;
  huffman.Init(entryStrings);
  huffman.WriteEncoding(writer);

  for (size_t i = 0; i < entries.size(); ++i)
  {
    TEntry const & e = entries[i];
    auto const & key = entryStrings[i];

    vector<uint8_t> buf;
    MemWriter<vector<uint8_t>> memWriter(buf);
    uint32_t numBits = huffman.EncodeAndWrite(memWriter, key);

    MemReader bitEncoding(&buf[0], buf.size());

    TNode * cur = AddToTrie(root, bitEncoding, numBits);
    cur->m_isFinal = true;
    cur->m_valueList.Append(e.GetValue());
    cur->m_edgeBuilder.AddValue(e.value_data(), e.value_size());
  }

  vector<TNode *> levelOrder = WriteInLevelOrder(root);

  vector<bool> trieTopology(2 * levelOrder.size());
  for (size_t i = 0; i < levelOrder.size(); ++i)
  {
    auto const & cur = levelOrder[i];
    if (cur->l)
      trieTopology[2 * i] = true;
    if (cur->r)
      trieTopology[2 * i + 1] = true;
  }

  {
    BitWriter<TWriter> bitWriter(writer);
    WriteVarUint(writer, levelOrder.size());
    for (size_t i = 0; i < 2 * levelOrder.size(); ++i)
      bitWriter.Write(trieTopology[i] ? 1 : 0, 1 /* numBits */);
  }

  vector<uint32_t> finalNodeIds;
  vector<uint32_t> offsetTable;
  vector<uint8_t> valueBuf;
  MemWriter<vector<uint8_t>> valueWriter(valueBuf);
  for (size_t i = 0; i < levelOrder.size(); ++i)
  {
    TNode const * nd = levelOrder[i];
    if (!nd->m_isFinal)
      continue;
    finalNodeIds.push_back(static_cast<uint32_t>(i));
    offsetTable.push_back(valueWriter.Pos());
    nd->m_valueList.Dump(valueWriter);
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

  for (uint8_t const b : valueBuf)
    writer.Write(&b, 1 /* numBytes */);

  // todo(@pimenov):
  // 1. Investigate the possibility of path compression (short edges + lcp table).
  // 2. It is highly probable that valueList will be only a list of feature ids in our
  //    future implementations of the search.
  //    We can then dispose of offsetTable and store finalNodeIds as another bit vector.
  //    Alternatively (and probably better), we can append a special symbol to all those
  //    strings that have non-empty value lists. We can then get the leaf number
  //    of a final node by doing rank0.

  DeleteTrie(root);
}

}  // namespace trie
