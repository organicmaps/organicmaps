#pragma once

#include "coding/bwt_coder.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"
#include "base/lru_cache.hpp"

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace coding
{
// Writes a set of strings in a format that allows to efficiently
// access blocks of strings. This means that access of individual
// strings may be inefficient, but access to a block of strings can be
// performed in O(length of all strings in the block + log(number of
// blocks)). The size of each block roughly equals to the |blockSize|,
// because the whole number of strings is packed into a single block.
//
// Format description:
// * first 8 bytes - little endian-encoded offset of the index section
// * data section - represents a catenated sequence of BWT-compressed blocks with
//   a sequence of individual string lengths in the block
// * index section - represents a delta-encoded sequence of
//   BWT-compressed blocks offsets intermixed with the number of
//   strings inside each block.
//
// All numbers except the first offset are varints.
template <typename Writer>
class BlockedTextStorageWriter
{
public:
  BlockedTextStorageWriter(Writer & writer, uint64_t blockSize)
    : m_writer(writer)
    , m_blockSize(blockSize)
    , m_startOffset(writer.Pos())
    , m_blocks(1)
  {
    CHECK(m_blockSize != 0, ());
    WriteToSink(m_writer, static_cast<uint64_t>(0));
    m_dataOffset = m_writer.Pos();
  }

  ~BlockedTextStorageWriter()
  {
    if (!m_lengths.empty())
      FlushPool(m_lengths, m_pool);

    if (m_blocks.back().IsEmpty())
      m_blocks.pop_back();

    {
      auto const currentOffset = m_writer.Pos();
      ASSERT_GREATER_OR_EQUAL(currentOffset, m_startOffset, ());
      m_writer.Seek(m_startOffset);
      WriteToSink(m_writer, static_cast<uint64_t>(currentOffset - m_startOffset));
      m_writer.Seek(currentOffset);
    }

    WriteVarUint(m_writer, m_blocks.size());

    uint64_t prevOffset = 0;
    for (auto const & block : m_blocks)
    {
      ASSERT_GREATER_OR_EQUAL(block.m_offset, prevOffset, ());
      WriteVarUint(m_writer, block.m_offset - prevOffset);

      ASSERT(!block.IsEmpty(), ());
      WriteVarUint(m_writer, block.m_subs);

      prevOffset = block.m_offset;
    }
  }

  void Append(std::string_view sv)
  {
    ASSERT(!m_blocks.empty(), ());

    ASSERT_LESS(m_pool.size(), m_blockSize, ());

    ++m_blocks.back().m_subs;
    m_pool.append(sv);
    m_lengths.push_back(sv.size());

    if (m_pool.size() >= m_blockSize)
    {
      FlushPool(m_lengths, m_pool);
      m_pool.clear();
      m_lengths.clear();
      m_blocks.emplace_back(m_writer.Pos() - m_dataOffset /* offset */, 0 /* subs */);
    }
  }

private:
  struct Block
  {
    Block() = default;
    Block(uint64_t offset, uint64_t subs) : m_offset(offset), m_subs(subs) {}

    bool IsEmpty() const { return m_subs == 0; }

    uint64_t m_offset = 0;  // offset of the block inside the sequence of compressed blocks
    uint64_t m_subs = 0;    // number of strings inside the block
  };

  void FlushPool(std::vector<uint64_t> const & lengths, std::string const & pool)
  {
    for (auto const & length : lengths)
      WriteVarUint(m_writer, length);
    BWTCoder::EncodeAndWriteBlock(m_writer, pool.size(), reinterpret_cast<uint8_t const *>(pool.c_str()));
  }

  Writer & m_writer;
  uint64_t const m_blockSize;
  uint64_t m_startOffset = 0;
  uint64_t m_dataOffset = 0;

  std::vector<Block> m_blocks;

  std::string m_pool;               // concatenated strings
  std::vector<uint64_t> m_lengths;  // lengths of strings inside the |m_pool|
};

class BlockedTextStorageIndex
{
public:
  struct BlockInfo
  {
    // Returns the index of the first string belonging to the block.
    uint64_t From() const { return m_from; }

    // Returns the index of the first string from the next block.
    uint64_t To() const { return m_from + m_subs; }

    uint64_t m_offset = 0;  // offset of the block from the beginning of the section
    uint64_t m_from = 0;    // index of the first string in the block
    uint64_t m_subs = 0;    // number of strings in the block
  };

  size_t GetNumBlockInfos() const { return m_blocks.size(); }
  size_t GetNumStrings() const { return m_blocks.empty() ? 0 : static_cast<size_t>(m_blocks.back().To()); }

  BlockInfo const & GetBlockInfo(size_t blockIx) const
  {
    ASSERT_LESS(blockIx, GetNumBlockInfos(), ());
    return m_blocks[blockIx];
  }

  // Returns the index of the block the |stringIx| belongs to.
  // Returns the number of blocks if there're no such block.
  size_t GetBlockIx(size_t stringIx) const
  {
    if (m_blocks.empty() || stringIx >= m_blocks.back().To())
      return GetNumBlockInfos();
    if (stringIx >= m_blocks.back().From())
      return GetNumBlockInfos() - 1;

    size_t lo = 0, hi = GetNumBlockInfos() - 1;
    while (lo + 1 != hi)
    {
      ASSERT_GREATER_OR_EQUAL(stringIx, m_blocks[lo].From(), ());
      ASSERT_LESS(stringIx, m_blocks[hi].From(), ());

      auto const mi = lo + (hi - lo) / 2;
      if (stringIx >= m_blocks[mi].From())
        lo = mi;
      else
        hi = mi;
    }

    ASSERT_GREATER_OR_EQUAL(stringIx, m_blocks[lo].From(), ());
    ASSERT_LESS(stringIx, m_blocks[hi].From(), ());
    return lo;
  }

  template <typename Reader>
  void Read(Reader & reader)
  {
    auto const indexOffset = ReadPrimitiveFromPos<uint64_t>(reader, 0);

    NonOwningReaderSource source(reader);
    source.Skip(indexOffset);

    auto const numBlocks = ReadVarUint<uint64_t>(source);
    m_blocks.assign(static_cast<size_t>(numBlocks), {});

    uint64_t prevOffset = 8;  // 8 bytes for the offset of the data section
    for (uint64_t i = 0; i < numBlocks; ++i)
    {
      auto const delta = ReadVarUint<uint64_t>(source);
      CHECK_GREATER_OR_EQUAL(prevOffset + delta, prevOffset, ());
      prevOffset += delta;

      auto & block = m_blocks[static_cast<size_t>(i)];
      block.m_offset = prevOffset;
      block.m_from = i == 0 ? 0 : m_blocks[static_cast<size_t>(i - 1)].To();
      block.m_subs = ReadVarUint<uint64_t>(source);
      CHECK_GREATER_OR_EQUAL(block.m_from + block.m_subs, block.m_from, ());
    }
  }

private:
  std::vector<BlockInfo> m_blocks;
};

class BlockedTextStorageReader
{
public:
  inline static size_t const kDefaultCacheSize = 32;

  BlockedTextStorageReader() : m_cache(kDefaultCacheSize) {}
  explicit BlockedTextStorageReader(size_t cacheSize) : m_cache(cacheSize) {}

  template <typename Reader>
  void InitializeIfNeeded(Reader & reader)
  {
    if (m_initialized)
      return;
    m_index.Read(reader);
    m_initialized = true;
  }

  size_t GetNumStrings() const
  {
    CHECK(m_initialized, ());
    return m_index.GetNumStrings();
  }

  template <typename Reader>
  std::string ExtractString(Reader & reader, size_t stringIx)
  {
    InitializeIfNeeded(reader);

    auto const blockIx = m_index.GetBlockIx(stringIx);
    CHECK_LESS(blockIx, m_index.GetNumBlockInfos(), ());

    auto const & bi = m_index.GetBlockInfo(blockIx);

    bool found;
    auto & entry = m_cache.Find(blockIx, found);
    if (!found)
    {
      NonOwningReaderSource source(reader);
      source.Skip(bi.m_offset);

      entry.m_value.clear();
      entry.m_subs.resize(static_cast<size_t>(bi.m_subs));

      uint64_t offset = 0;
      for (size_t i = 0; i < entry.m_subs.size(); ++i)
      {
        auto & sub = entry.m_subs[i];
        sub.m_offset = offset;
        sub.m_length = ReadVarUint<uint64_t>(source);
        CHECK_GREATER_OR_EQUAL(sub.m_offset + sub.m_length, sub.m_offset, ());
        offset += sub.m_length;
      }
      entry.m_value = BWTCoder::ReadAndDecodeBlock(source);
    }

    ASSERT_GREATER_OR_EQUAL(stringIx, bi.From(), ());
    ASSERT_LESS(stringIx, bi.To(), ());

    stringIx -= bi.From();
    ASSERT_LESS(stringIx, entry.m_subs.size(), ());

    auto const & si = entry.m_subs[stringIx];
    auto const & value = entry.m_value;
    ASSERT_LESS_OR_EQUAL(si.m_offset + si.m_length, value.size(), ());
    auto const beg = value.begin() + si.m_offset;
    return std::string(beg, beg + si.m_length);
  }

private:
  struct StringInfo
  {
    StringInfo() = default;
    StringInfo(uint64_t offset, uint64_t length) : m_offset(offset), m_length(length) {}

    uint64_t m_offset = 0;  // offset of the string inside the decompressed block
    uint64_t m_length = 0;  // length of the string
  };

  struct CacheEntry
  {
    BWTCoder::BufferT m_value;       // concatenation of the strings
    std::vector<StringInfo> m_subs;  // indices of individual strings
  };

  BlockedTextStorageIndex m_index;
  LruCache<size_t, CacheEntry> m_cache;
  bool m_initialized = false;
};

template <typename Reader>
class BlockedTextStorage
{
public:
  explicit BlockedTextStorage(Reader & reader) : m_reader(reader) { m_storage.InitializeIfNeeded(m_reader); }

  size_t GetNumStrings() const { return m_storage.GetNumStrings(); }
  std::string ExtractString(size_t stringIx) { return m_storage.ExtractString(m_reader, stringIx); }

private:
  BlockedTextStorageReader m_storage;
  Reader & m_reader;
};
}  // namespace coding
