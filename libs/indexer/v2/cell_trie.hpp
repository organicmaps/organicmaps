#pragma once

#include "indexer/v2/ids_packet.hpp"
#include "indexer/v2/succinct_wrapper.hpp"

#include "coding/byte_stream.hpp"
#include "coding/reader_wrapper.hpp"
#include "coding/writer.hpp"

#include "geometry/cellid.hpp"

#include "base/assert.hpp"
#include "base/bits.hpp"
#include "base/buffer_vector.hpp"
#include "base/macros.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace idx::v2
{
/// Common part of the disk driven multimap from the cells of a quadtree to ids and its builder.
/// A cell is addressed by its number in the tree of the depth levels, see m2::CellId::ToInt64().
class CellTrieBase
{
public:
  // The level, the root and the bits of a cell make one sorting key of the builder.
  static constexpr uint8_t kMaxDepth = 25;
  // A block has at most this power of two of tokens.
  static constexpr uint8_t kMaxBlockLog = 4;
  // The default number of ids that the builder puts into a block.
  static constexpr uint32_t kBlockIds = 32;

  struct Header
  {
    uint8_t m_depth = 0;
    uint8_t m_roots = 0;
    uint8_t m_reserved[6] = {};
    uint64_t m_tokens = 0;
    // The sizes of the parts after the header and the levels. The packets take the rest.
    uint64_t m_masksSize = 0;
    uint64_t m_endingSize = 0;
    uint64_t m_offsetsSize = 0;
  };
  static_assert(sizeof(Header) == 40, "");

  // The tokens of a level make blocks of 2^m_blockLog tokens, the last block may be shorter.
  struct Level
  {
    uint64_t m_firstToken = 0;
    uint32_t m_firstBlock = 0;
    uint8_t m_blockLog = 0;
    uint8_t m_reserved[3] = {};
  };
  static_assert(sizeof(Level) == 16, "");

  using Cell = m2::CellId<kMaxDepth>;
};

/// Disk driven multimap from the cells of a quadtree to ids, for the lookups by the intervals of
/// the cell numbers. It keeps several such maps, the roots, and a lookup takes the first ones of
/// them: the scale buckets of a geometry index up to the scale of the lookup.
///
/// The cells with ids and their parents are the nodes of a trie: the bits of a parent are the
/// prefix of the bits of its children. The nodes are stored in the level order, the roots first,
/// so the children of a node, and all its descendants of one level, are neighbours.
/// The ids of a node are a token. The tokens follow the order of the nodes.
/// Format:
/// - Header.
/// - Level for every level of the tree.
/// - RsBitsVector with four bits per node above the last level: the existing children.
/// - RsBitsVector with a bit per node: the node has a token.
/// - EfBitsVector with the offsets of the blocks of tokens in the packets.
/// - Packets of the tokens, @see IdsPacket. The first packet of a block has the first id as is,
///   the next ones have its distance to the first id of the previous packet. So a level of
///   short tokens has long blocks: few offsets and short first ids; a token of a level of long
///   tokens is read without passing many other ones.
/// Every part before the packets is padded to eight bytes.
///
/// The trie is not loaded into memory: its object keeps the header and the levels, the vectors
/// and the packets are read from the reader on demand. So a trie is for one thread, like its
/// reader: every reader of a file has its own trie, which is cheap to make.
template <class TReader>
class CellTrie : public CellTrieBase
{
  // A part of the reader.
  using PartT = SubReaderWrapper<TReader const>;

public:
  /// |reader| has to outlive the trie.
  explicit CellTrie(TReader const & reader)
    : m_header(ReadHeader(reader))
    , m_masksReader(PartT(&reader).SubReader(GetMasksPos(), m_header.m_masksSize))
    , m_endingReader(PartT(&reader).SubReader(GetMasksPos() + m_header.m_masksSize, m_header.m_endingSize))
    , m_offsetsReader(PartT(&reader).SubReader(GetPacketsPos() - m_header.m_offsetsSize, m_header.m_offsetsSize))
    , m_packetsReader(PartT(&reader).SubReader(GetPacketsPos(), reader.Size() - GetPacketsPos()))
    , m_masks(m_masksReader)
    , m_ending(m_endingReader)
    , m_offsets(m_offsetsReader)
  {
    reader.Read(sizeof(Header), m_levels.data(), m_header.m_depth * sizeof(Level));
    for (uint8_t level = 0; level < m_header.m_depth; ++level)
      CHECK_LESS_OR_EQUAL(m_levels[level].m_blockLog, kMaxBlockLog, (level));
    CHECK_EQUAL(m_ending.num_ones(), m_header.m_tokens, ());
  }

  Header const & GetHeader() const { return m_header; }
  uint64_t GetNodesCount() const { return m_ending.size(); }
  // The position of the packets in the reader.
  uint64_t GetPacketsPos() const
  {
    return GetMasksPos() + m_header.m_masksSize + m_header.m_endingSize + m_header.m_offsetsSize;
  }

  /// Looks up the intervals of the cell numbers. Not thread safe: it keeps the last found path of
  /// every root, so the lookup of a cell near the previous one takes a few steps.
  class Cursor
  {
  public:
    /// |trie| has to outlive the cursor.
    explicit Cursor(CellTrie const & trie) : m_trie(trie), m_paths(trie.m_header.m_roots)
    {
      for (uint32_t root = 0; root < m_paths.size(); ++root)
      {
        m_paths[root].m_nodes[0] = root;
        // A root with ids has a token or children.
        if (trie.m_ending[root] || trie.m_masks.rank(4 * root) != trie.m_masks.rank(4 * (root + 1)))
          m_roots.push_back(root);
      }
    }

    /// Calls fn(id) for the ids of the cells with the numbers in [beg, end) in the roots
    /// [0, rootsEnd). An id comes once for every such cell that has it.
    template <class Fn>
    void ForEach(uint64_t beg, uint64_t end, uint32_t rootsEnd, Fn && fn)
    {
      uint8_t const depth = m_trie.m_header.m_depth;
      beg = std::max(beg, uint64_t{1});
      end = std::min(end, Cell::Root().SubTreeSize(depth) + 1);
      if (beg >= end)
        return;

      // The interval is a sequence of whole subtrees and of single cells, whose subtrees it cuts.
      Cell cell = Cell::FromInt64(static_cast<int64_t>(beg), depth);
      while (beg < end)
      {
        uint64_t const size = cell.SubTreeSize(depth);
        bool const isSubtree = beg + size <= end;
        uint8_t const level = static_cast<uint8_t>(cell.Level());
        for (uint32_t const root : m_roots)
        {
          if (root >= rootsEnd)
            break;
          uint64_t const node = FindNode(m_paths[root], cell);
          if (node == kNoNode)
            continue;
          if (isSubtree && level + 1 < depth)
            ForEachInSubtree(node, level, fn);
          else
            ForEachInNode(node, level, fn);
        }

        if (!isSubtree)
        {
          // The first child.
          ++beg;
          cell = cell.Child(0);
          continue;
        }

        // The next cell after the subtree.
        beg += size;
        while (cell.Level() > 0 && cell.WhichChildOfParent() == 3)
          cell = cell.Parent();
        if (cell.Level() == 0)
          break;
        cell = Cell::FromBitsAndLevel(cell.Bits() + 1, cell.Level());
      }
    }

  private:
    struct Path
    {
      // The nodes of a found cell and of its parents by the level.
      std::array<uint32_t, kMaxDepth> m_nodes = {};
      uint64_t m_bits = 0;
      uint8_t m_level = 0;
    };

    uint64_t FindNode(Path & path, Cell const & cell) const
    {
      uint8_t const cellLevel = static_cast<uint8_t>(cell.Level());
      uint64_t const cellBits = cell.Bits();
      uint8_t level = std::min(cellLevel, path.m_level);
      uint64_t const diff = (cellBits >> 2 * (cellLevel - level)) ^ (path.m_bits >> 2 * (path.m_level - level));
      if (diff != 0)
        level -= (std::bit_width(diff) + 1) / 2;
      else if (level == cellLevel)
        return path.m_nodes[level];

      auto const & masks = m_trie.m_masks;
      uint64_t node = path.m_nodes[level];
      while (level < cellLevel)
      {
        ++level;
        uint64_t const bits = cellBits >> 2 * (cellLevel - level);
        uint64_t const child = 4 * node + (bits & 3);
        if (!masks[child])
        {
          path.m_level = level - 1;
          path.m_bits = bits >> 2;
          return kNoNode;
        }
        node = masks.rank(child) + m_trie.m_header.m_roots;
        path.m_nodes[level] = static_cast<uint32_t>(node);
      }
      path.m_level = level;
      path.m_bits = cellBits;
      return node;
    }

    template <class Fn>
    void ForEachInNode(uint64_t node, uint8_t level, Fn && fn) const
    {
      if (!m_trie.m_ending[node])
        return;
      uint64_t const token = m_trie.m_ending.rank(node);
      ForEachInTokens(level, token, token + 1, fn);
    }

    // The descendants of a node on a level are neighbours, so are their tokens.
    template <class Fn>
    void ForEachInSubtree(uint64_t node, uint8_t level, Fn && fn) const
    {
      uint64_t first = node;
      uint64_t last = node + 1;
      while (true)
      {
        uint64_t const firstToken = m_trie.m_ending.rank(first);
        uint64_t const lastToken = m_trie.m_ending.rank(last);
        if (firstToken != lastToken)
          ForEachInTokens(level, firstToken, lastToken, fn);

        if (++level == m_trie.m_header.m_depth)
          break;
        first = m_trie.m_masks.rank(4 * first) + m_trie.m_header.m_roots;
        last = m_trie.m_masks.rank(4 * last) + m_trie.m_header.m_roots;
        if (first == last)
          break;
      }
    }

    // Calls fn(id) for the ids of the tokens [first, last) of a level.
    template <class Fn>
    void ForEachInTokens(uint8_t level, uint64_t first, uint64_t last, Fn && fn) const
    {
      Level const & info = m_trie.m_levels[level];
      uint64_t const firstBlock = info.m_firstBlock + ((first - info.m_firstToken) >> info.m_blockLog);
      uint64_t const lastBlock = info.m_firstBlock + ((last - 1 - info.m_firstToken) >> info.m_blockLog);
      // The two offsets of one block are neighbours: one select finds them.
      auto const [offset, end] = firstBlock == lastBlock ? m_trie.m_offsets.select_range(firstBlock)
                                                         : std::make_pair(m_trie.m_offsets.select(firstBlock),
                                                                          m_trie.m_offsets.select(lastBlock + 1));
      // The blocks are taken in one read.
      buffer_vector<uint8_t, 1024> buffer(end - offset);
      m_trie.m_packetsReader.Read(offset, buffer.data(), buffer.size());
      ReadTokens(buffer.data(), info, first, last, fn);
    }

    // |packets| start with the block of the token |first|.
    template <class Fn>
    static void ReadTokens(uint8_t const * packets, Level const & info, uint64_t first, uint64_t last, Fn && fn)
    {
      uint64_t const blockMask = (uint64_t{1} << info.m_blockLog) - 1;
      ArrayByteSource src(packets);
      uint32_t previous = 0;
      uint32_t link = 0;
      // The index of a token on its level.
      uint64_t index = (first - info.m_firstToken) & ~blockMask;
      first -= info.m_firstToken;
      last -= info.m_firstToken;
      for (; index < last; ++index)
      {
        bool const isBlockStart = (index & blockMask) == 0;
        auto const singleton = isBlockStart ? IdsPacket::Singleton::Split : IdsPacket::Singleton::Count;
        auto const toFirstId = [&](uint64_t value)
        {
          previous = static_cast<uint32_t>(isBlockStart ? value : previous + bits::ZigZagDecode(value));
          return previous;
        };

        if (index < first)
          toFirstId(IdsPacket::Skip(src, singleton));
        else
          VERIFY(IdsPacket::Read(src, singleton, link, toFirstId, fn), ());
      }
    }

    CellTrie const & m_trie;
    std::vector<Path> m_paths;
    // The roots that have ids.
    std::vector<uint32_t> m_roots;
  };

private:
  static constexpr uint64_t kNoNode = std::numeric_limits<uint64_t>::max();

  static Header ReadHeader(TReader const & reader)
  {
    Header header;
    reader.Read(0, &header, sizeof(header));
    CHECK(header.m_depth >= 2 && header.m_depth <= kMaxDepth, (header.m_depth));
    CHECK_GREATER(header.m_roots, 0, ());
    // The parts before the packets are in the reader.
    uint64_t const size = reader.Size();
    CHECK(header.m_masksSize <= size && header.m_endingSize <= size && header.m_offsetsSize <= size, (size));
    CHECK_LESS_OR_EQUAL(sizeof(Header) + header.m_depth * sizeof(Level) + header.m_masksSize + header.m_endingSize +
                            header.m_offsetsSize,
                        size, ());
    return header;
  }

  uint64_t GetMasksPos() const { return sizeof(Header) + m_header.m_depth * sizeof(Level); }

  Header m_header;
  std::array<Level, kMaxDepth> m_levels;
  // The parts of the reader: of the vectors and of the packets.
  PartT m_masksReader;
  PartT m_endingReader;
  PartT m_offsetsReader;
  PartT m_packetsReader;
  RsBitsVector<PartT> m_masks;
  RsBitsVector<PartT> m_ending;
  EfBitsVector<PartT> m_offsets;

  DISALLOW_COPY_AND_MOVE(CellTrie);
};

class CellTrieBuilder : public CellTrieBase
{
public:
  // A block gets about |blockIds| ids, a level with more ids per token gets no blocks.
  CellTrieBuilder(uint8_t depth, uint8_t roots, uint32_t blockIds = kBlockIds)
    : m_depth(depth)
    , m_roots(roots)
    , m_blockIds(blockIds)
  {
    CHECK(depth >= 2 && depth <= kMaxDepth, (depth));
    CHECK_GREATER(roots, 0, ());
  }

  void Add(uint32_t root, uint64_t cellNumber, uint32_t id)
  {
    ASSERT_LESS(root, m_roots, ());
    Cell const cell = Cell::FromInt64(static_cast<int64_t>(cellNumber), m_depth);
    m_items.push_back({(static_cast<uint64_t>(cell.Level()) << kLevelShift) | MakeNode(root, cell.Bits()), id});
  }

  template <class TWriter>
  void Flush(TWriter & writer)
  {
    std::sort(m_items.begin(), m_items.end());
    m_items.erase(std::unique(m_items.begin(), m_items.end()), m_items.end());

    // The nodes of every level in the order of the roots and of the bits.
    std::vector<std::vector<uint64_t>> nodes(m_depth);
    for (auto const & item : m_items)
    {
      auto & level = nodes[item.first >> kLevelShift];
      uint64_t const node = item.first & kNodeMask;
      if (level.empty() || level.back() != node)
        level.push_back(node);
    }
    for (size_t level = m_depth - 1; level > 0; --level)
    {
      auto & parents = nodes[level - 1];
      size_t const size = parents.size();
      for (auto const node : nodes[level])
      {
        uint64_t const parent = GetParent(node);
        if (parents.size() == size || parents.back() != parent)
          parents.push_back(parent);
      }
      std::inplace_merge(parents.begin(), parents.begin() + size, parents.end());
      parents.erase(std::unique(parents.begin(), parents.end()), parents.end());
    }
    // Every root is a node, also an empty one.
    nodes[0].clear();
    for (uint32_t root = 0; root < m_roots; ++root)
      nodes[0].push_back(MakeNode(root, 0));

    RsBitsVectorBuilder masks;
    for (size_t level = 0; level + 1 < m_depth; ++level)
    {
      auto child = nodes[level + 1].begin();
      auto const end = nodes[level + 1].end();
      for (auto const node : nodes[level])
      {
        std::array<bool, 4> mask = {};
        for (; child != end && GetParent(*child) == node; ++child)
          mask[*child & 3] = true;
        for (bool const bit : mask)
          masks.PushBack(bit);
      }
      ASSERT(child == end, ());
    }

    // The tokens and the ids of every level.
    std::vector<std::pair<uint64_t, uint64_t>> counts(m_depth);
    for (size_t i = 0; i < m_items.size(); ++i)
    {
      auto & count = counts[m_items[i].first >> kLevelShift];
      if (i == 0 || m_items[i].first != m_items[i - 1].first)
        ++count.first;
      ++count.second;
    }

    RsBitsVectorBuilder ending;
    EfBitsVectorBuilder offsets;
    std::vector<Level> levels(m_depth);
    std::vector<uint8_t> packets;
    MemWriter packetsWriter(packets);
    std::vector<uint32_t> ids;
    uint64_t tokens = 0;
    uint32_t blocks = 0;
    uint32_t previous = 0;
    auto item = m_items.begin();
    for (size_t level = 0; level < m_depth; ++level)
    {
      auto & info = levels[level];
      info.m_firstToken = tokens;
      info.m_firstBlock = blocks;
      if (counts[level].first != 0)
      {
        uint64_t const blockTokens = m_blockIds * counts[level].first / counts[level].second;
        info.m_blockLog = static_cast<uint8_t>(std::min<int>(kMaxBlockLog, std::bit_width(blockTokens | 1) - 1));
      }

      for (auto const node : nodes[level])
      {
        uint64_t const key = (uint64_t{level} << kLevelShift) | node;
        ids.clear();
        for (; item != m_items.end() && item->first == key; ++item)
          ids.push_back(item->second);
        ending.PushBack(!ids.empty());
        if (ids.empty())
          continue;

        bool const isBlockStart = ((tokens - info.m_firstToken) & ((uint64_t{1} << info.m_blockLog) - 1)) == 0;
        if (isBlockStart)
        {
          ++blocks;
          offsets.PushBack(packets.size());
          IdsPacket::Write(packetsWriter, ids, ids.front(), IdsPacket::Singleton::Split);
        }
        else
        {
          IdsPacket::Write(packetsWriter, ids, bits::ZigZagEncode(int64_t{ids.front()} - int64_t{previous}),
                           IdsPacket::Singleton::Count);
        }
        previous = ids.front();
        ++tokens;
      }
    }
    ASSERT(item == m_items.end(), ());
    offsets.PushBack(packets.size());

    auto const masksBuffer = FlushToBuffer(masks);
    auto const endingBuffer = FlushToBuffer(ending);
    auto const offsetsBuffer = FlushToBuffer(offsets);

    Header header;
    header.m_depth = m_depth;
    header.m_roots = m_roots;
    header.m_tokens = tokens;
    header.m_masksSize = masksBuffer.size();
    header.m_endingSize = endingBuffer.size();
    header.m_offsetsSize = offsetsBuffer.size();
    writer.Write(&header, sizeof(header));
    writer.Write(levels.data(), levels.size() * sizeof(Level));
    writer.Write(masksBuffer.data(), masksBuffer.size());
    writer.Write(endingBuffer.data(), endingBuffer.size());
    writer.Write(offsetsBuffer.data(), offsetsBuffer.size());
    writer.Write(packets.data(), packets.size());
  }

private:
  static constexpr uint8_t kRootShift = 48;
  static constexpr uint8_t kLevelShift = 56;
  static constexpr uint64_t kNodeMask = (uint64_t{1} << kLevelShift) - 1;
  static constexpr uint64_t kBitsMask = (uint64_t{1} << kRootShift) - 1;

  static uint64_t MakeNode(uint32_t root, uint64_t bits) { return (uint64_t{root} << kRootShift) | bits; }
  static uint64_t GetParent(uint64_t node) { return (node & ~kBitsMask) | ((node & kBitsMask) >> 2); }

  template <class TBuilder>
  static std::vector<uint8_t> FlushToBuffer(TBuilder & builder)
  {
    std::vector<uint8_t> buffer;
    MemWriter writer(buffer);
    builder.Flush(writer);
    // A frozen succinct structure takes whole words, so the next part is aligned too.
    ASSERT_EQUAL(buffer.size() % 8, 0, ());
    return buffer;
  }

  uint8_t m_depth;
  uint8_t m_roots;
  uint32_t m_blockIds;
  // The level, the root and the bits of a cell, and an id.
  std::vector<std::pair<uint64_t, uint32_t>> m_items;
};
}  // namespace idx::v2
