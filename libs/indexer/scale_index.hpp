#pragma once

#include "indexer/interval_index.hpp"
#include "indexer/v2/cell_trie.hpp"

#include "coding/reader.hpp"
#include "coding/var_serial_vector.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

/// Index bucket <--> Draw scale range.
/// Using default one-to-one mapping.
class ScaleIndexBase
{
public:
  static uint32_t GetBucketsCount() { return 18; }
  static uint32_t BucketByScale(int scale) { return static_cast<uint32_t>(scale); }
  /// @return Range like [x, y).
  static std::pair<uint32_t, uint32_t> ScaleRangeForBucket(uint32_t bucket) { return {bucket, bucket + 1}; }

  /// The format of the index.
  enum class Version : uint32_t
  {
    /// A serial vector with an interval index for every bucket. It has no header and starts with
    /// the number of the buckets.
    V1 = 1,
    /// Header, then a trie of the cells with a root for every bucket, @see idx::v2::CellTrie.
    V2 = 2,
    Latest = V2
  };

  // The bytes "GIDX". The number of the buckets of the version 1 is never that big.
  static constexpr uint32_t kSignature = 0x58444947;

  // Keep the size of the header a multiple of 8 bytes. The sections of a map are aligned to 8 bytes,
  // so the 64-bit words of the trie's vectors after such a header are aligned too, and a word never
  // crosses the pages of a reader's cache.
  struct Header
  {
    uint32_t m_signature = kSignature;
    Version m_version = Version::Latest;
  };
  static_assert(sizeof(Header) == 8, "");
};

/// Reads the geometry index of a map in any version. Nothing of the index is loaded: a lookup
/// reads what it needs from the reader. Not thread safe, like its reader.
template <class Reader>
class ScaleIndex : public ScaleIndexBase
{
public:
  using Trie = idx::v2::CellTrie<Reader>;

  explicit ScaleIndex(Reader const & reader)
  {
    if (ReadPrimitiveFromPos<uint32_t>(reader, 0) != kSignature)
    {
      ReaderSource<Reader> source(reader);
      VarSerialVectorReader<Reader> treesReader(source);
      for (uint32_t i = 0; i < treesReader.Size(); ++i)
        m_IndexForScale.push_back(std::make_unique<IntervalIndexT>(treesReader.SubReader(i)));
      return;
    }

    Header header;
    reader.Read(0, &header, sizeof(header));
    CHECK(header.m_version == Version::V2, (static_cast<uint32_t>(header.m_version)));
    m_lookup = std::make_unique<TrieLookup>(reader.SubReader(sizeof(Header), reader.Size() - sizeof(Header)));
  }

  Version GetVersion() const { return m_lookup ? Version::V2 : Version::V1; }

  /// Calls fn(featureId) for the features of the cells [beg, end) that are visible on |scale|.
  /// A feature comes once for every such cell of its covering.
  template <class FnT>
  void ForEachInIntervalAndScale(uint64_t beg, uint64_t end, int scale, FnT && fn) const
  {
    auto const scaleBucket = BucketByScale(scale);
    if (m_lookup)
    {
      if (scaleBucket < m_lookup->m_trie.GetHeader().m_roots)
        m_lookup->m_cursor.ForEach(beg, end, scaleBucket + 1, fn);
      return;
    }

    if (scaleBucket < m_IndexForScale.size())
      for (size_t i = 0; i <= scaleBucket; ++i)
        m_IndexForScale[i]->ForEach([&fn](uint64_t /* cell */, uint32_t featureId) { fn(featureId); }, beg, end);
  }

private:
  using IntervalIndexT = IntervalIndex<Reader, uint32_t>;

  struct TrieLookup
  {
    explicit TrieLookup(Reader const & reader) : m_reader(reader), m_trie(m_reader), m_cursor(m_trie) {}

    Reader m_reader;
    Trie const m_trie;
    // Keeps the paths of the last lookup: the next one is usually near.
    typename Trie::Cursor m_cursor;
  };

  // The version 1.
  std::vector<std::unique_ptr<IntervalIndexT>> m_IndexForScale;
  // The version 2.
  std::unique_ptr<TrieLookup> m_lookup;
};
