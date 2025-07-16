#pragma once

#include "coding/bit_streams.hpp"
#include "coding/varint.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <map>
#include <memory>
#include <type_traits>
#include <vector>

namespace coding
{
class HuffmanCoder
{
public:
  class Freqs
  {
  public:
    using Table = std::map<uint32_t, uint32_t>;

    Freqs() = default;

    template <typename... Args>
    Freqs(Args const &... args)
    {
      Add(args...);
    }

    void Add(strings::UniString const & s) { Add(s.begin(), s.end()); }

    void Add(std::string const & s) { Add(s.begin(), s.end()); }

    template <typename T>
    void Add(T const * begin, T const * const end)
    {
      static_assert(std::is_integral<T>::value, "");
      AddImpl(begin, end);
    }

    template <typename It>
    void Add(It begin, It const end)
    {
      static_assert(std::is_integral<typename It::value_type>::value, "");
      AddImpl(begin, end);
    }

    template <typename T>
    void Add(std::vector<T> const & v)
    {
      for (auto const & e : v)
        Add(std::begin(e), std::end(e));
    }

    Table const & GetTable() const { return m_table; }

  private:
    template <typename It>
    void AddImpl(It begin, It const end)
    {
      static_assert(sizeof(*begin) <= 4, "");
      for (; begin != end; ++begin)
        ++m_table[static_cast<uint32_t>(*begin)];
    }

    Table m_table;
  };

  // A Code encodes a path to a leaf. It is read starting from
  // the least significant bit.
  struct Code
  {
    uint32_t bits;
    size_t len;

    Code() : bits(0), len(0) {}
    Code(uint32_t bits, size_t len) : bits(bits), len(len) {}

    bool operator<(Code const & o) const
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
  template <typename... Args>
  void Init(Args const &... args)
  {
    Clear();
    BuildHuffmanTree(Freqs(args...));
    BuildTables(m_root, 0);
  }

  void Clear();

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

  template <typename TWriter, typename T>
  uint32_t EncodeAndWrite(TWriter & writer, T const * begin, T const * end) const
  {
    static_assert(std::is_integral<T>::value, "");
    return EncodeAndWriteImpl(writer, begin, end);
  }

  template <typename TWriter, typename It>
  uint32_t EncodeAndWrite(TWriter & writer, It begin, It end) const
  {
    static_assert(std::is_integral<typename It::value_type>::value, "");
    return EncodeAndWriteImpl(writer, begin, end);
  }

  template <typename TWriter>
  uint32_t EncodeAndWrite(TWriter & writer, std::string const & s) const
  {
    return EncodeAndWrite(writer, s.begin(), s.end());
  }

  // Returns the number of bits written AFTER the size, i.e. the number
  // of bits that the encoded string consists of.
  template <typename TWriter>
  uint32_t EncodeAndWrite(TWriter & writer, strings::UniString const & s) const
  {
    return EncodeAndWrite(writer, s.begin(), s.end());
  }

  template <typename TSource, typename OutIt>
  OutIt ReadAndDecode(TSource & src, OutIt out) const
  {
    BitReader<TSource> bitReader(src);
    size_t sz = static_cast<size_t>(ReadVarUint<uint32_t, TSource>(src));
    for (size_t i = 0; i < sz; ++i)
      *out++ = ReadAndDecode(bitReader);
    return out;
  }

  template <typename TSource>
  strings::UniString ReadAndDecode(TSource & src) const
  {
    strings::UniString result;
    ReadAndDecode(src, std::back_inserter(result));
    return result;
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
      : l(nullptr)
      , r(nullptr)
      , symbol(symbol)
      , freq(freq)
      , depth(0)
      , isLeaf(isLeaf)
    {}
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

  template <typename TWriter, typename It>
  uint32_t EncodeAndWriteImpl(TWriter & writer, It begin, It end) const
  {
    static_assert(sizeof(*begin) <= 4, "");

    size_t const d = base::asserted_cast<size_t>(std::distance(begin, end));
    BitWriter<TWriter> bitWriter(writer);
    WriteVarUint(writer, d);
    uint32_t sz = 0;
    for (; begin != end; ++begin)
      sz += EncodeAndWrite(bitWriter, static_cast<uint32_t>(*begin));
    return sz;
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

  void BuildHuffmanTree(Freqs const & freqs);

  // Properly sets the depth field in the subtree rooted at root.
  // It is easier to do it after the tree is built.
  void SetDepths(Node * root, uint32_t depth);

  Node * m_root;
  std::map<Code, uint32_t> m_decoderTable;
  std::map<uint32_t, Code> m_encoderTable;
};
}  // namespace coding
