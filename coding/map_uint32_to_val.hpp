#pragma once

#include "coding/files_container.hpp"
#include "coding/memory_region.hpp"
#include "coding/reader.hpp"
#include "coding/succinct_mapper.hpp"
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
// 2                    block size          2
// 4                    positions offset    4
// 8                    variables offset    4
// 12                   end of section      4
// 16                   identifiers table   positions offset - 16
// positions offset     positions table     variables offset - positions offset
// variables offset     variables blocks    end of section - variables offset
//
// Identifiers table is a bit-vector with rank-select table, where set
// bits denote that values for the corresponding features are in the
// table.  Identifiers table is stored in the native endianness.
//
// Positions table is an Elias-Fano table where each entry corresponds
// to the start position of the variables block.
//
// Variables is a sequence of blocks, where each block (with the
// exception of the last one) is a sequence of kBlockSize variables
// encoded by block encoding callback.
//
// On Get call m_blockSize consecutive variables are decoded and cached in RAM.

template <typename Value>
class MapUint32ToValue
{
  // 0 - initial version.
  // 1 - added m_blockSize instead of m_endianess.
  static uint16_t constexpr kLastVersion = 1;

public:
  using ReadBlockCallback = std::function<void(NonOwningReaderSource &, uint32_t, std::vector<Value> &)>;

  struct Header
  {
    uint16_t Read(Reader & reader)
    {
      NonOwningReaderSource source(reader);
      auto const version = ReadPrimitiveFromSource<uint16_t>(source);
      m_blockSize = ReadPrimitiveFromSource<uint16_t>(source);
      if (version == 0)
        m_blockSize = 64;

      m_positionsOffset = ReadPrimitiveFromSource<uint32_t>(source);
      m_variablesOffset = ReadPrimitiveFromSource<uint32_t>(source);
      m_endOffset = ReadPrimitiveFromSource<uint32_t>(source);
      return version;
    }

    void Write(Writer & writer)
    {
      WriteToSink(writer, kLastVersion);
      WriteToSink(writer, m_blockSize);
      WriteToSink(writer, m_positionsOffset);
      WriteToSink(writer, m_variablesOffset);
      WriteToSink(writer, m_endOffset);
    }

    uint16_t m_blockSize = 0;
    uint32_t m_positionsOffset = 0;
    uint32_t m_variablesOffset = 0;
    uint32_t m_endOffset = 0;
  };

  MapUint32ToValue(Reader & reader, ReadBlockCallback const & readBlockCallback)
    : m_reader(reader)
    , m_readBlockCallback(readBlockCallback)
  {}

  /// @name Tries to get |value| for key identified by |id|.
  /// @returns false if table does not have entry for this id.
  /// @{
  [[nodiscard]] bool Get(uint32_t id, Value & value)
  {
    if (id >= m_ids.size() || !m_ids[id])
      return false;

    uint32_t const rank = static_cast<uint32_t>(m_ids.rank(id));
    uint32_t const base = rank / m_header.m_blockSize;
    uint32_t const offset = rank % m_header.m_blockSize;

    auto & entry = m_cache[base];
    if (entry.empty())
      entry = GetImpl(rank, m_header.m_blockSize);

    value = entry[offset];
    return true;
  }

  [[nodiscard]] bool GetThreadsafe(uint32_t id, Value & value) const
  {
    if (id >= m_ids.size() || !m_ids[id])
      return false;

    uint32_t const rank = static_cast<uint32_t>(m_ids.rank(id));
    uint32_t const offset = rank % m_header.m_blockSize;

    auto const entry = GetImpl(rank, offset + 1);

    value = entry[offset];
    return true;
  }
  /// @}

  // Loads MapUint32ToValue instance. Note that |reader| must be alive
  // until the destruction of loaded table. Returns nullptr if
  // MapUint32ToValue can't be loaded.
  // It's guaranteed that |readBlockCallback| will not be called for empty block.
  static std::unique_ptr<MapUint32ToValue> Load(Reader & reader, ReadBlockCallback const & readBlockCallback)
  {
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
      CHECK(Get(j, value), (i, j));
      fn(j, value);
    }
  }

  uint64_t Count() const { return m_ids.num_ones(); }

private:
  /// @param[in] upperSize Read until this size. Can be one of: \n
  /// - m_header.m_blockSize for the regular Get version with cache \n
  /// - index + 1 for the GetThreadsafe version without cache, to break when needed element is readed \n
  std::vector<Value> GetImpl(uint32_t rank, uint32_t upperSize) const
  {
    uint32_t const base = rank / m_header.m_blockSize;
    auto const start = m_offsets.select(base);
    auto const end = base + 1 < m_offsets.num_ones() ? m_offsets.select(base + 1) + m_header.m_variablesOffset
                                                     : m_header.m_endOffset;
    NonOwningReaderSource src(m_reader, m_header.m_variablesOffset + start, end);

    // Important! Client should read while src.Size() > 0 and max |upperSize| number of elements.
    std::vector<Value> values;
    m_readBlockCallback(src, upperSize, values);
    return values;
  }

  bool Init()
  {
    auto const version = m_header.Read(m_reader);
    if (version > kLastVersion)
    {
      LOG(LERROR, ("Unsupported version =", version, "Last known version =", kLastVersion));
      return false;
    }

    {
      uint32_t const idsSize = m_header.m_positionsOffset - sizeof(m_header);
      std::vector<uint8_t> data(idsSize);
      m_reader.Read(sizeof(m_header), data.data(), data.size());
      m_idsRegion = std::make_unique<CopiedMemoryRegion>(std::move(data));

      coding::MapVisitor visitor(m_idsRegion->ImmutableData());
      m_ids.map(visitor);
    }

    {
      uint32_t const offsetsSize = m_header.m_variablesOffset - m_header.m_positionsOffset;
      std::vector<uint8_t> data(offsetsSize);
      m_reader.Read(m_header.m_positionsOffset, data.data(), data.size());
      m_offsetsRegion = std::make_unique<CopiedMemoryRegion>(std::move(data));

      coding::MapVisitor visitor(m_offsetsRegion->ImmutableData());
      m_offsets.map(visitor);
    }

    return true;
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
  template <class WriterT>
  void Freeze(WriterT & writer, WriteBlockCallback const & writeBlockCallback, uint16_t blockSize = 64) const
  {
    typename Map::Header header;
    header.m_blockSize = blockSize;

    auto const startOffset = writer.Pos();
    header.Write(writer);

    {
      uint64_t const numBits = m_ids.empty() ? 0 : m_ids.back() + 1;

      succinct::bit_vector_builder builder(numBits);
      for (auto const & id : m_ids)
        builder.set(id, true);

      coding::FreezeVisitor<WriterT> visitor(writer);
      succinct::rs_bit_vector(&builder).map(visitor);
    }

    std::vector<uint32_t> offsets;
    std::vector<uint8_t> variables;

    {
      MemWriter<std::vector<uint8_t>> writer(variables);
      for (size_t i = 0; i < m_values.size(); i += blockSize)
      {
        offsets.push_back(static_cast<uint32_t>(variables.size()));

        auto const endOffset = std::min(i + blockSize, m_values.size());
        CHECK_GREATER(endOffset, i, ());
        writeBlockCallback(writer, m_values.cbegin() + i, m_values.cbegin() + endOffset);
      }
    }

    {
      succinct::elias_fano::elias_fano_builder builder(offsets.empty() ? 0 : offsets.back() + 1, offsets.size());
      for (auto const & offset : offsets)
        builder.push_back(offset);

      header.m_positionsOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);
      coding::FreezeVisitor<WriterT> visitor(writer);
      succinct::elias_fano(&builder).map(visitor);
    }

    {
      header.m_variablesOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);
      writer.Write(variables.data(), variables.size());
      header.m_endOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);
    }

    auto const endOffset = writer.Pos();

    writer.Seek(startOffset);
    header.Write(writer);
    writer.Seek(endOffset);
  }

private:
  std::vector<Value> m_values;
  std::vector<uint32_t> m_ids;
};
