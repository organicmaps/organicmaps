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
bool CentersTable::Get(uint32_t id, m2::PointD & center)
{
  m2::PointU pointu;
  if (!m_map->Get(id, pointu))
    return false;

  center = PointUToPointD(pointu, m_codingParams.GetCoordBits());
  return true;
}

// CentersTable ------------------------------------------------------------------------------------
// static
unique_ptr<CentersTable> CentersTable::Load(Reader & reader,
                                            serial::GeometryCodingParams const & codingParams)
{
  auto table = make_unique<CentersTable>();
  if (!table->Init(reader, codingParams))
    return {};
  return table;
}

bool CentersTable::Init(Reader & reader, serial::GeometryCodingParams const & codingParams)
{
  m_codingParams = codingParams;
  // Decodes block encoded by writeBlockCallback from CentersTableBuilder::Freeze.
  auto const readBlockCallback = [&](NonOwningReaderSource & source, uint32_t blockSize,
                                     vector<m2::PointU> & values) {
    values.resize(blockSize);
    uint64_t delta = ReadVarUint<uint64_t>(source);
    values[0] = coding::DecodePointDeltaFromUint(delta, m_codingParams.GetBasePoint());

    for (size_t i = 1; i < blockSize && source.Size() > 0; ++i)
    {
      delta = ReadVarUint<uint64_t>(source);
      values[i] = coding::DecodePointDeltaFromUint(delta, values[i - 1]);
    }
  };

  m_map = Map::Load(reader, readBlockCallback);
  return m_map != nullptr;
}

// CentersTableBuilder -----------------------------------------------------------------------------
void CentersTableBuilder::Put(uint32_t featureId, m2::PointD const & center)
{
  m_builder.Put(featureId, PointDToPointU(center, m_codingParams.GetCoordBits()));
}

void CentersTableBuilder::Freeze(Writer & writer) const
{
  // Each center is encoded as delta from some prediction.
  // For the first center in the block map base point is used as a prediction, for all other
  // centers in the block previous center is used as a prediction.
  auto const writeBlockCallback = [&](Writer & w, vector<m2::PointU>::const_iterator begin,
                                      vector<m2::PointU>::const_iterator end) {
    uint64_t delta = coding::EncodePointDeltaAsUint(*begin, m_codingParams.GetBasePoint());
    WriteVarUint(w, delta);
    auto prevIt = begin;
    for (auto it = begin + 1; it != end; ++it)
    {
      delta = coding::EncodePointDeltaAsUint(*it, *prevIt);
      WriteVarUint(w, delta);
      prevIt = it;
    }
  };

  m_builder.Freeze(writer, writeBlockCallback);
}
}  // namespace search
