#include "indexer/centers_table.hpp"

#include "indexer/feature_processor.hpp"

#include "coding/endianness.hpp"
#include "coding/file_container.hpp"
#include "coding/geometry_coding.hpp"
#include "coding/memory_region.hpp"
#include "coding/point_coding.hpp"
#include "coding/reader.hpp"
#include "coding/succinct_mapper.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"

#include <unordered_map>

#include "3party/succinct/elias_fano.hpp"
#include "3party/succinct/rs_bit_vector.hpp"

using namespace std;

namespace search
{
namespace
{
template <typename TCont>
void EndiannessAwareMap(bool endiannesMismatch, CopiedMemoryRegion & region, TCont & cont)
{
  TCont c;
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

// V0 of CentersTable.  Has the following format:
//
// File offset (bytes)  Field name          Field size (bytes)
// 0                    common header       4
// 4                    positions offset    4
// 8                    deltas offset       4
// 12                   end of section      4
// 16                   identifiers table   positions offset - 16
// positions offset     positions table     deltas offset - positions offset
// deltas offset        deltas blocks       end of section - deltas offset
//
// All offsets are in little-endian format.
//
// Identifiers table is a bit-vector with rank-select table, where set
// bits denote that centers for the corresponding features are in
// table.  Identifiers table is stored in the native endianness.
//
// Positions table is a Elias-Fano table, where each entry corresponds
// to the start position of the centers block. Positions table is
// stored in the native endianness.
//
// Deltas is a sequence of blocks, where each block (with the
// exception of the last one) is a sequence of kBlockSize varuints,
// where each varuint represents an encoded delta of the center from
// some prediction. For the first center in the block map base point
// is used as a prediction, for all other centers in the block
// previous center is used as a prediction. This allows to decode
// block of kBlockSize consecutive centers at one syscall, and cache
// them in RAM.
class CentersTableV0 : public CentersTable
{
public:
  static const uint32_t kBlockSize = 64;

  struct Header
  {
    void Read(Reader & reader)
    {
      m_base.Read(reader);

      NonOwningReaderSource source(reader);
      source.Skip(sizeof(m_base));
      m_positionsOffset = ReadPrimitiveFromSource<uint32_t>(source);
      m_deltasOffset = ReadPrimitiveFromSource<uint32_t>(source);
      m_endOffset = ReadPrimitiveFromSource<uint32_t>(source);
    }

    void Write(Writer & writer)
    {
      m_base.Write(writer);

      WriteToSink(writer, m_positionsOffset);
      WriteToSink(writer, m_deltasOffset);
      WriteToSink(writer, m_endOffset);
    }

    bool IsValid() const
    {
      if (!m_base.IsValid())
      {
        LOG(LERROR, ("Base header is not valid!"));
        return false;
      }
      if (m_positionsOffset < sizeof(m_header))
      {
        LOG(LERROR, ("Positions before header:", m_positionsOffset, sizeof(m_header)));
        return false;
      }
      if (m_deltasOffset < m_positionsOffset)
      {
        LOG(LERROR, ("Deltas before positions:", m_deltasOffset, m_positionsOffset));
        return false;
      }
      if (m_endOffset < m_deltasOffset)
      {
        LOG(LERROR, ("End of section before deltas:", m_endOffset, m_deltasOffset));
        return false;
      }
      return true;
    }

    CentersTable::Header m_base;
    uint32_t m_positionsOffset = 0;
    uint32_t m_deltasOffset = 0;
    uint32_t m_endOffset = 0;
  };

  static_assert(sizeof(Header) == 16, "Wrong header size.");

  CentersTableV0(Reader & reader, serial::GeometryCodingParams const & codingParams)
    : m_reader(reader), m_codingParams(codingParams)
  {
  }

  // CentersTable overrides:
  bool Get(uint32_t id, m2::PointD & center) override
  {
    if (id >= m_ids.size() || !m_ids[id])
      return false;
    uint32_t const rank = static_cast<uint32_t>(m_ids.rank(id));
    uint32_t const base = rank / kBlockSize;
    uint32_t const offset = rank % kBlockSize;

    auto & entry = m_cache[base];
    if (entry.empty())
    {
      entry.resize(kBlockSize);

      auto const start = m_offsets.select(base);
      auto const end = base + 1 < m_offsets.num_ones()
                           ? m_offsets.select(base + 1)
                           : m_header.m_endOffset - m_header.m_deltasOffset;

      vector<uint8_t> data(end - start);

      m_reader.Read(m_header.m_deltasOffset + start, data.data(), data.size());

      MemReader mreader(data.data(), data.size());
      NonOwningReaderSource msource(mreader);

      uint64_t delta = ReadVarUint<uint64_t>(msource);
      entry[0] = coding::DecodePointDeltaFromUint(delta, m_codingParams.GetBasePoint());

      for (size_t i = 1; i < kBlockSize && msource.Size() > 0; ++i)
      {
        delta = ReadVarUint<uint64_t>(msource);
        entry[i] = coding::DecodePointDeltaFromUint(delta, entry[i - 1]);
      }
    }

    center = PointUToPointD(entry[offset], m_codingParams.GetCoordBits());
    return true;
  }

private:
  // CentersTable overrides:
  bool Init() override
  {
    m_header.Read(m_reader);

    if (!m_header.IsValid())
      return false;

    bool const isHostBigEndian = IsBigEndianMacroBased();
    bool const isDataBigEndian = m_header.m_base.m_endianness == 1;
    bool const endiannesMismatch = isHostBigEndian != isDataBigEndian;

    {
      uint32_t const idsSize = m_header.m_positionsOffset - sizeof(m_header);
      vector<uint8_t> data(idsSize);
      m_reader.Read(sizeof(m_header), data.data(), data.size());
      m_idsRegion = make_unique<CopiedMemoryRegion>(move(data));
      EndiannessAwareMap(endiannesMismatch, *m_idsRegion, m_ids);
    }

    {
      uint32_t const offsetsSize = m_header.m_deltasOffset - m_header.m_positionsOffset;
      vector<uint8_t> data(offsetsSize);
      m_reader.Read(m_header.m_positionsOffset, data.data(), data.size());
      m_offsetsRegion = make_unique<CopiedMemoryRegion>(move(data));
      EndiannessAwareMap(endiannesMismatch, *m_offsetsRegion, m_offsets);
    }

    return true;
  }

private:
  Header m_header;
  Reader & m_reader;
  serial::GeometryCodingParams const m_codingParams;

  unique_ptr<CopiedMemoryRegion> m_idsRegion;
  unique_ptr<CopiedMemoryRegion> m_offsetsRegion;

  succinct::rs_bit_vector m_ids;
  succinct::elias_fano m_offsets;

  unordered_map<uint32_t, vector<m2::PointU>> m_cache;
};
}  // namespace

// CentersTable::Header ----------------------------------------------------------------------------
void CentersTable::Header::Read(Reader & reader)
{
  NonOwningReaderSource source(reader);
  m_version = ReadPrimitiveFromSource<uint16_t>(source);
  m_endianness = ReadPrimitiveFromSource<uint16_t>(source);
}

void CentersTable::Header::Write(Writer & writer)
{
  WriteToSink(writer, m_version);
  WriteToSink(writer, m_endianness);
}

bool CentersTable::Header::IsValid() const
{
  if (m_endianness > 1)
    return false;
  return true;
}

// CentersTable ------------------------------------------------------------------------------------
unique_ptr<CentersTable> CentersTable::Load(Reader & reader,
                                            serial::GeometryCodingParams const & codingParams)
{
  uint16_t const version = ReadPrimitiveFromPos<uint16_t>(reader, 0 /* pos */);
  if (version != 0)
    return unique_ptr<CentersTable>();

  // Only single version of centers table is supported now.  If you
  // need to implement new versions of CentersTable, implement
  // dispatching based on first-four-bytes version.
  unique_ptr<CentersTable> table = make_unique<CentersTableV0>(reader, codingParams);
  if (!table->Init())
    return unique_ptr<CentersTable>();
  return table;
}

// CentersTableBuilder -----------------------------------------------------------------------------
void CentersTableBuilder::Put(uint32_t featureId, m2::PointD const & center)
{
  if (!m_ids.empty())
    CHECK_LESS(m_ids.back(), featureId, ());

  m_centers.push_back(PointDToPointU(center, m_codingParams.GetCoordBits()));
  m_ids.push_back(featureId);
}

void CentersTableBuilder::Freeze(Writer & writer) const
{
  CentersTableV0::Header header;

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

  vector<uint32_t> offsets;
  vector<uint8_t> deltas;

  {
    MemWriter<vector<uint8_t>> writer(deltas);
    for (size_t i = 0; i < m_centers.size(); i += CentersTableV0::kBlockSize)
    {
      offsets.push_back(static_cast<uint32_t>(deltas.size()));

      uint64_t delta = coding::EncodePointDeltaAsUint(m_centers[i], m_codingParams.GetBasePoint());
      WriteVarUint(writer, delta);
      for (size_t j = i + 1; j < i + CentersTableV0::kBlockSize && j < m_centers.size(); ++j)
      {
        delta = coding::EncodePointDeltaAsUint(m_centers[j], m_centers[j - 1]);
        WriteVarUint(writer, delta);
      }
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
    header.m_deltasOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);
    writer.Write(deltas.data(), deltas.size());
    header.m_endOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);
  }

  auto const endOffset = writer.Pos();

  writer.Seek(startOffset);
  CHECK_EQUAL(header.m_base.m_endianness, 0, ("|m_endianness| should be set to little-endian."));
  header.Write(writer);
  writer.Seek(endOffset);
}
}  // namespace search
