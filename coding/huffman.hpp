#pragma once

#include "coding/bit_streams.hpp"
#include "coding/varint.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/map.hpp"
#include "std/queue.hpp"
#include "std/vector.hpp"

namespace coding
{
class HuffmanCoder
{
public:
  // A Code encodes a path to a leaf. It is read starting from
  // the least significant bit.
  struct Code
  {
    uint32_t bits;
    size_t len;

    Code() : bits(0), len(0) {}
    Code(uint32_t bits, size_t len) : bits(bits), len(len) {}

    bool operator<(const Code & o) const
    {
      if (bits != o.bits)
        return bits < o.bits;
      return len < o.len;
    }
  };

  HuffmanCoder() : m_root(nullptr) {}
  ~HuffmanCoder();

  // Internally builds a Huffman tree and makes
  // the EncodeAndWrite and ReadAndDecode methods available.
  void Init(vector<strings::UniString> const & data);

  // One way to store the encoding would be
  // -- the succinct representation of the topology of Huffman tree;
  // -- the list of symbols that are stored in the leaves, as varuints in delta encoding.
  // This would probably be an overkill.
  template <typename TWriter>
  void WriteEncoding(TWriter & writer)
  {
    // @todo Do not waste space, use BitWriter.
    WriteVarUint(writer, m_decoderTable.size());
    for (auto const & kv : m_decoderTable)
    {
      WriteVarUint(writer, kv.first.bits);
      WriteVarUint(writer, kv.first.len);
      WriteVarUint(writer, kv.second);
    }
  }

  template <typename TSource>
  void ReadEncoding(TSource & src)
  {
    DeleteHuffmanTree(m_root);
    m_root = new Node(0 /* symbol */, 0 /* freq */, false /* isLeaf */);

    m_encoderTable.clear();
    m_decoderTable.clear();

    size_t sz = static_cast<size_t>(ReadVarUint<uint32_t, TSource>(src));
    for (size_t i = 0; i < sz; ++i)
    {
      uint32_t bits = ReadVarUint<uint32_t, TSource>(src);
      uint32_t len = ReadVarUint<uint32_t, TSource>(src);
      uint32_t symbol = ReadVarUint<uint32_t, TSource>(src);
      Code code(bits, len);

      m_encoderTable[symbol] = code;
      m_decoderTable[code] = symbol;

      Node * cur = m_root;
      for (size_t j = 0; j < len; ++j)
      {
        if (((bits >> j) & 1) == 0)
        {
          if (!cur->l)
            cur->l = new Node(0 /* symbol */, 0 /* freq */, false /* isLeaf */);
          cur = cur->l;
        }
        else
        {
          if (!cur->r)
            cur->r = new Node(0 /* symbol */, 0 /* freq */, false /* isLeaf */);
          cur = cur->r;
        }
        cur->depth = j + 1;
      }
      cur->isLeaf = true;
      cur->symbol = symbol;
    }
  }

  bool Encode(uint32_t symbol, Code & code) const;
  bool Decode(Code const & code, uint32_t & symbol) const;

  // Returns the number of bits written AFTER the size, i.e. the number
  // of bits that the encoded string consists of.
  template <typename TWriter>
  uint32_t EncodeAndWrite(TWriter & writer, strings::UniString const & s) const
  {
    BitWriter<TWriter> bitWriter(writer);
    WriteVarUint(writer, s.size());
    uint32_t sz = 0;
    for (size_t i = 0; i < s.size(); ++i)
      sz += EncodeAndWrite(bitWriter, static_cast<uint32_t>(s[i]));
    return sz;
  }

  template <typename TSource>
  strings::UniString ReadAndDecode(TSource & src) const
  {
    BitReader<TSource> bitReader(src);
    size_t sz = static_cast<size_t>(ReadVarUint<uint32_t, TSource>(src));
    vector<strings::UniChar> v(sz);
    for (size_t i = 0; i < sz; ++i)
      v[i] = static_cast<strings::UniChar>(ReadAndDecode(bitReader));
    return strings::UniString(v.begin(), v.end());
  }

private:

  struct Node
  {
    Node *l, *r;
    uint32_t symbol;
    uint32_t freq;
    size_t depth;
    bool isLeaf;

    Node(uint32_t symbol, uint32_t freq, bool isLeaf)
        : l(nullptr), r(nullptr), symbol(symbol), freq(freq), depth(0), isLeaf(isLeaf)
    {
    }
  };

  struct NodeComparator
  {
    bool operator()(Node const * const a, Node const * const b) const
    {
      if (a->freq != b->freq)
        return a->freq > b->freq;
      return a->symbol > b->symbol;
    }
  };

  // No need to clump the interface: keep private the methods
  // that encode one symbol only.
  template <typename TWriter>
  size_t EncodeAndWrite(BitWriter<TWriter> & bitWriter, uint32_t symbol) const
  {
    Code code;
    CHECK(Encode(symbol, code), ());
    size_t fullBytes = code.len / CHAR_BIT;
    size_t rem = code.len % CHAR_BIT;
    for (size_t i = 0; i < fullBytes; ++i)
    {
      bitWriter.Write(code.bits & 0xFF, CHAR_BIT);
      code.bits >>= CHAR_BIT;
    }
    bitWriter.Write(code.bits, rem);
    return code.len;
  }

  template <typename TSource>
  uint32_t ReadAndDecode(BitReader<TSource> & bitReader) const
  {
    Node * cur = m_root;
    while (cur)
    {
      if (cur->isLeaf)
        return cur->symbol;
      uint8_t bit = bitReader.Read(1);
      if (bit == 0)
        cur = cur->l;
      else
        cur = cur->r;
    }
    CHECK(false, ("Could not decode a Huffman-encoded symbol."));
    return 0;
  }

  // Converts a Huffman tree into the more convenient representation
  // of encoding and decoding tables.
  void BuildTables(Node * root, uint32_t path);

  void DeleteHuffmanTree(Node * root);

  // Builds a fixed Huffman tree for a collection of strings::UniStrings.
  // UniString is always UTF-32. Every code point is treated as a symbol for the encoder.
  template <typename TIter>
  void BuildHuffmanTree(TIter const & beg, TIter const & end)
  {
    if (beg == end)
    {
      m_root = nullptr;
      return;
    }

    // One would need more than 2^32 symbols to build a code that long.
    // On the other hand, 32 is short enough for our purposes, so do not
    // try to shrink the trees beyond this threshold.
    uint32_t const kMaxDepth = 32;

    map<uint32_t, uint32_t> freqs;
    for (auto it = beg; it != end; ++it)
    {
      auto const & e = *it;
      for (size_t i = 0; i < e.size(); ++i)
      {
        ++freqs[static_cast<uint32_t>(e[i])];
      }
    }

    priority_queue<Node *, vector<Node *>, NodeComparator> pq;
    for (auto const & e : freqs)
      pq.push(new Node(e.first, e.second, true /* isLeaf */));

    if (pq.empty())
    {
      m_root = nullptr;
      return;
    }

    while (pq.size() > 1)
    {
      auto a = pq.top();
      pq.pop();
      auto b = pq.top();
      pq.pop();
      if (a->symbol > b->symbol)
        swap(a, b);
      // Give it the smaller symbol to make the resulting encoding more predictable.
      auto ab = new Node(a->symbol, a->freq + b->freq, false /* isLeaf */);
      ab->l = a;
      ab->r = b;
      CHECK_LESS_OR_EQUAL(a->depth, kMaxDepth, ());
      CHECK_LESS_OR_EQUAL(b->depth, kMaxDepth, ());
      pq.push(ab);
    }

    m_root = pq.top();
    pq.pop();

    SetDepths(m_root, 0);
  }

  // Properly sets the depth field in the subtree rooted at root.
  // It is easier to do it after the tree is built.
  void SetDepths(Node * root, uint32_t depth);

  Node * m_root;  // m_pRoot?
  map<Code, uint32_t> m_decoderTable;
  map<uint32_t, Code> m_encoderTable;
};

}  // namespace coding
