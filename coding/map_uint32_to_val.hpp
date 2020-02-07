#pragma once

#include "coding/endianness.hpp"
#include "coding/files_container.hpp"
#include "coding/memory_region.hpp"
#include "coding/reader.hpp"
#include "coding/succinct_mapper.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif

#include "3party/succinct/elias_fano.hpp"
#include "3party/succinct/rs_bit_vector.hpp"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>

// A data structure that allows storing a map from small 32-bit integers (the main use
// case is feature ids of a single mwm) to arbitrary values and accessing this map
// with a small RAM footprint.
//
// Format:
// File offset (bytes)  Field name          Field size (bytes)
// 0                    version             2
// 2                    endianness          2
// 4                    positions offset    4
// 8                    variables offset    4
// 12                   end of section      4
// 16                   identifiers table   positions offset - 16
// positions offset     positions table     variables offset - positions offset
// variables offset     variables blocks    end of section - variables offset
//
// Version and endianness are always stored in the little-endian format.
// 0 value of endianness means little-endian, whereas 1 means big-endian.
//
// All offsets are in the little-endian format.
//
// Identifiers table is a bit-vector with rank-select table, where set
// bits denote that values for the corresponding features are in the
// table.  Identifiers table is stored in the native endianness.
//
// Positions table is an Elias-Fano table where each entry corresponds
// to the start position of the variables block. Positions table is
// stored in the native endianness.
//
// Variables is a sequence of blocks, where each block (with the
// exception of the last one) is a sequence of kBlockSize variables
// encoded by block encoding callback.
//
// On Get call kBlockSize consecutive variables are decoded and cached in RAM. Get is not
// threadsafe.
//
// GetThreadsafe does not use cache.

template <typename Value>
class MapUint32ToValue
{
public:
  using ReadBlockCallback =
      std::function<void(NonOwningReaderSource &, uint32_t, std::vector<Value> &)>;

  static uint32_t constexpr kBlockSize = 64;

  struct Header
  {
    void Read(Reader & reader)
    {
      NonOwningReaderSource source(reader);
      m_version = ReadPrimitiveFromSource<uint16_t>(source);
      m_endianness = ReadPrimitiveFromSource<uint16_t>(source);

      m_positionsOffset = ReadPrimitiveFromSource<uint32_t>(source);
      m_variablesOffset = ReadPrimitiveFromSource<uint32_t>(source);
      m_endOffset = ReadPrimitiveFromSource<uint32_t>(source);
    }

    void Write(Writer & writer)
    {
      WriteToSink(writer, m_version);
      WriteToSink(writer, m_endianness);
      WriteToSink(writer, m_positionsOffset);
      WriteToSink(writer, m_variablesOffset);
      WriteToSink(writer, m_endOffset);
    }

    bool IsValid() const
    {
      if (m_version != 0)
      {
        LOG(LERROR, ("Unknown version."));
        return false;
      }

      if (m_endianness > 1)
      {
        LOG(LERROR, ("Wrong endianness value."));
        return false;
      }

      if (m_positionsOffset < sizeof(m_header))
      {
        LOG(LERROR, ("Positions before header:", m_positionsOffset, sizeof(m_header)));
        return false;
      }

      if (m_variablesOffset < m_positionsOffset)
      {
        LOG(LERROR, ("Deltas before positions:", m_variablesOffset, m_positionsOffset));
        return false;
      }

      if (m_endOffset < m_variablesOffset)
      {
        LOG(LERROR, ("End of section before variables:", m_endOffset, m_variablesOffset));
        return false;
      }

      return true;
    }

    uint16_t m_version = 0;
    uint16_t m_endianness = 0;
    uint32_t m_positionsOffset = 0;
    uint32_t m_variablesOffset = 0;
    uint32_t m_endOffset = 0;
  };

  static_assert(sizeof(Header) == 16, "Wrong header size");

  MapUint32ToValue(Reader & reader, ReadBlockCallback const & readBlockCallback)
    : m_reader(reader), m_readBlockCallback(readBlockCallback)
  {
  }

  ~MapUint32ToValue() = default;

  // Tries to get |value| for key identified by |id|.  Returns
  // false if table does not have entry for this id.
  WARN_UNUSED_RESULT bool Get(uint32_t id, Value & value)
  {
    if (id >= m_ids.size() || !m_ids[id])
      return false;

    uint32_t const rank = static_cast<uint32_t>(m_ids.rank(id));
    uint32_t const base = rank / kBlockSize;
    uint32_t const offset = rank % kBlockSize;

    auto & entry = m_cache[base];
    if (entry.empty())
      entry = GetImpl(id);

    value = entry[offset];
    return true;
  }

  // Tries to get |value| for key identified by |id|.  Returns
  // false if table does not have entry for this id.
  WARN_UNUSED_RESULT bool GetThreadsafe(uint32_t id, Value & value) const
  {
    if (id >= m_ids.size() || !m_ids[id])
      return false;

    uint32_t const rank = static_cast<uint32_t>(m_ids.rank(id));
    uint32_t const offset = rank % kBlockSize;

    auto const entry = GetImpl(id);

    value = entry[offset];
    return true;
  }

  // Loads MapUint32ToValue instance. Note that |reader| must be alive
  // until the destruction of loaded table. Returns nullptr if
  // MapUint32ToValue can't be loaded.
  // It's guaranteed that |readBlockCallback| will not be called for empty block.
  static std::unique_ptr<MapUint32ToValue> Load(Reader & reader,
                                                ReadBlockCallback const & readBlockCallback)
  {
    uint16_t const version = ReadPrimitiveFromPos<uint16_t>(reader, 0 /* pos */);
    if (version != 0)
      return {};

    // Only single version of centers table is supported now.  If you need to implement new
    // versions, implement dispatching based on first-four-bytes version.
    auto table = std::make_unique<MapUint32ToValue>(reader, readBlockCallback);
    if (!table->Init())
      return {};
    return table;
  }

  template <typename Fn>
  void ForEach(Fn && fn)
  {
    for (uint64_t i = 0; i < m_ids.num_ones(); ++i)
    {
      auto const j = static_cast<uint32_t>(m_ids.select(i));
      Value value;
      bool const ok = Get(j, value);
      CHECK(ok, ());
      fn(j, value);
    }
  }

  uint64_t Count() const { return m_ids.num_ones(); }

private:
  std::vector<Value> GetImpl(uint32_t id) const
  {
    ASSERT_LESS(id, m_ids.size(), ());
    ASSERT(m_ids[id], ());

    uint32_t const rank = static_cast<uint32_t>(m_ids.rank(id));
    uint32_t const base = rank / kBlockSize;

    std::vector<Value> values(kBlockSize);

    auto const start = m_offsets.select(base);
    auto const end = base + 1 < m_offsets.num_ones()
                         ? m_offsets.select(base + 1)
                         : m_header.m_endOffset - m_header.m_variablesOffset;

    std::vector<uint8_t> data(static_cast<size_t>(end - start));

    m_reader.Read(m_header.m_variablesOffset + start, data.data(), data.size());

    MemReader mreader(data.data(), data.size());
    NonOwningReaderSource msource(mreader);

    m_readBlockCallback(msource, kBlockSize, values);
    return values;
  }

  bool Init()
  {
    m_header.Read(m_reader);

    if (!m_header.IsValid())
      return false;

    bool const isHostBigEndian = IsBigEndianMacroBased();
    bool const isDataBigEndian = m_header.m_endianness == 1;
    bool const endiannesMismatch = isHostBigEndian != isDataBigEndian;

    {
      uint32_t const idsSize = m_header.m_positionsOffset - sizeof(m_header);
      std::vector<uint8_t> data(idsSize);
      m_reader.Read(sizeof(m_header), data.data(), data.size());
      m_idsRegion = std::make_unique<CopiedMemoryRegion>(move(data));
      EndiannessAwareMap(endiannesMismatch, *m_idsRegion, m_ids);
    }

    {
      uint32_t const offsetsSize = m_header.m_variablesOffset - m_header.m_positionsOffset;
      std::vector<uint8_t> data(offsetsSize);
      m_reader.Read(m_header.m_positionsOffset, data.data(), data.size());
      m_offsetsRegion = std::make_unique<CopiedMemoryRegion>(move(data));
      EndiannessAwareMap(endiannesMismatch, *m_offsetsRegion, m_offsets);
    }

    return true;
  }

  template <typename Cont>
  void EndiannessAwareMap(bool endiannesMismatch, CopiedMemoryRegion & region, Cont & cont)
  {
    Cont c;
    if (endiannesMismatch)
    {
      coding::ReverseMapVisitor visitor(region.MutableData());
      c.map(visitor);
    }
    else
    {
      coding::MapVisitor visitor(region.ImmutableData());
      c.map(visitor);
    }

    c.swap(cont);
  }

  Header m_header;
  Reader & m_reader;

  std::unique_ptr<CopiedMemoryRegion> m_idsRegion;
  std::unique_ptr<CopiedMemoryRegion> m_offsetsRegion;

  succinct::rs_bit_vector m_ids;
  succinct::elias_fano m_offsets;

  ReadBlockCallback m_readBlockCallback;

  std::unordered_map<uint32_t, std::vector<Value>> m_cache;
};

template <typename Value>
class MapUint32ToValueBuilder
{
public:
  using Iter = typename std::vector<Value>::const_iterator;
  using WriteBlockCallback = std::function<void(Writer &, Iter, Iter)>;
  using Map = MapUint32ToValue<Value>;

  void Put(uint32_t id, Value value)
  {
    if (!m_ids.empty())
      CHECK_LESS(m_ids.back(), id, ());

    m_values.push_back(value);
    m_ids.push_back(id);
  }

  // It's guaranteed that |writeBlockCallback| will not be called for empty block.
  void Freeze(Writer & writer, WriteBlockCallback const & writeBlockCallback) const
  {
    typename Map::Header header;

    auto const startOffset = writer.Pos();
    header.Write(writer);

    {
      uint64_t const numBits = m_ids.empty() ? 0 : m_ids.back() + 1;

      succinct::bit_vector_builder builder(numBits);
      for (auto const & id : m_ids)
        builder.set(id, true);

      coding::FreezeVisitor<Writer> visitor(writer);
      succinct::rs_bit_vector(&builder).map(visitor);
    }

    std::vector<uint32_t> offsets;
    std::vector<uint8_t> variables;

    {
      MemWriter<std::vector<uint8_t>> writer(variables);
      for (size_t i = 0; i < m_values.size(); i += Map::kBlockSize)
      {
        offsets.push_back(static_cast<uint32_t>(variables.size()));

        auto const endOffset = std::min(i + Map::kBlockSize, m_values.size());
        CHECK_GREATER(endOffset, i, ());
        writeBlockCallback(writer, m_values.cbegin() + i, m_values.cbegin() + endOffset);
      }
    }

    {
      succinct::elias_fano::elias_fano_builder builder(offsets.empty() ? 0 : offsets.back() + 1,
                                                       offsets.size());
      for (auto const & offset : offsets)
        builder.push_back(offset);

      header.m_positionsOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);
      coding::FreezeVisitor<Writer> visitor(writer);
      succinct::elias_fano(&builder).map(visitor);
    }

    {
      header.m_variablesOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);
      writer.Write(variables.data(), variables.size());
      header.m_endOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);
    }

    auto const endOffset = writer.Pos();

    writer.Seek(startOffset);
    CHECK_EQUAL(header.m_endianness, 0, ("|m_endianness| should be set to little-endian."));
    header.Write(writer);
    writer.Seek(endOffset);
  }

private:
  std::vector<Value> m_values;
  std::vector<uint32_t> m_ids;
};
