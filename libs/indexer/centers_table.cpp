#include "indexer/centers_table.hpp"
#include "indexer/feature_processor.hpp"

#include "coding/files_container.hpp"
#include "coding/geometry_coding.hpp"
#include "coding/point_coding.hpp"
#include "coding/reader.hpp"
#include "coding/succinct_mapper.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"

namespace search
{

void CentersTable::Header::Read(Reader & reader)
{
  NonOwningReaderSource source(reader);
  m_version = static_cast<Version>(ReadPrimitiveFromSource<uint8_t>(source));
  CHECK_EQUAL(static_cast<uint8_t>(m_version), static_cast<uint8_t>(Version::V1), ());
  m_geometryParamsOffset = ReadPrimitiveFromSource<uint32_t>(source);
  m_geometryParamsSize = ReadPrimitiveFromSource<uint32_t>(source);
  m_centersOffset = ReadPrimitiveFromSource<uint32_t>(source);
  m_centersSize = ReadPrimitiveFromSource<uint32_t>(source);
}

bool CentersTable::Get(uint32_t id, m2::PointD & center)
{
  m2::PointU pointu;
  if (!m_map->Get(id, pointu))
    return false;

  if (m_version == Version::V0)
    center = PointUToPointD(pointu, m_codingParams.GetCoordBits());
  else if (m_version == Version::V1)
    center = PointUToPointD(pointu, m_codingParams.GetCoordBits(), m_limitRect);
  else
    CHECK(false, ("Unknown CentersTable format."));

  return true;
}

// CentersTable ------------------------------------------------------------------------------------
// static
std::unique_ptr<CentersTable> CentersTable::LoadV0(Reader & reader, serial::GeometryCodingParams const & codingParams)
{
  auto table = std::make_unique<CentersTable>();
  table->m_version = Version::V0;
  if (!table->Init(reader, codingParams, {} /* limitRect */))
    return {};
  return table;
}

std::unique_ptr<CentersTable> CentersTable::LoadV1(Reader & reader)
{
  auto table = std::make_unique<CentersTable>();
  table->m_version = Version::V1;

  Header header;
  header.Read(reader);

  NonOwningReaderSource src(reader, header.m_geometryParamsOffset,
                            header.m_geometryParamsOffset + header.m_geometryParamsSize);

  serial::GeometryCodingParams codingParams;
  codingParams.Load(src);
  auto minX = ReadPrimitiveFromSource<uint32_t>(src);
  auto minY = ReadPrimitiveFromSource<uint32_t>(src);
  auto maxX = ReadPrimitiveFromSource<uint32_t>(src);
  auto maxY = ReadPrimitiveFromSource<uint32_t>(src);
  m2::RectD limitRect(PointUToPointD({minX, minY}, kPointCoordBits), PointUToPointD({maxX, maxY}, kPointCoordBits));

  table->m_centersSubreader = reader.CreateSubReader(header.m_centersOffset, header.m_centersSize);
  if (!table->m_centersSubreader)
    return {};
  if (!table->Init(*(table->m_centersSubreader), codingParams, limitRect))
    return {};
  return table;
}

bool CentersTable::Init(Reader & reader, serial::GeometryCodingParams const & codingParams, m2::RectD const & limitRect)
{
  m_codingParams = codingParams;
  m_limitRect = limitRect;
  // Decodes block encoded by writeBlockCallback from CentersTableBuilder::Freeze.
  auto const readBlockCallback =
      [&](NonOwningReaderSource & source, uint32_t blockSize, std::vector<m2::PointU> & values)
  {
    values.reserve(blockSize);

    auto prev = m_codingParams.GetBasePoint();
    while (source.Size() > 0)
    {
      auto const pt = coding::DecodePointDeltaFromUint(ReadVarUint<uint64_t>(source), prev);
      values.push_back(pt);
      prev = pt;
    }
  };

  m_map = Map::Load(reader, readBlockCallback);
  return m_map != nullptr;
}

// CentersTableBuilder -----------------------------------------------------------------------------
void CentersTableBuilder::SetGeometryParams(m2::RectD const & limitRect, double pointAccuracy)
{
  CHECK(limitRect.IsValid(), (limitRect));
  auto const coordBits = GetCoordBits(limitRect, pointAccuracy);
  m_codingParams = serial::GeometryCodingParams(coordBits, limitRect.Center());
  m_limitRect = limitRect;
}

void CentersTableBuilder::Put(uint32_t featureId, m2::PointD const & center)
{
  m_builder.Put(featureId, PointDToPointU(center, m_codingParams.GetCoordBits(), m_limitRect));
}

// Each center is encoded as delta from some prediction.
// For the first center in the block map base point is used as a prediction, for all other
// centers in the block previous center is used as a prediction.
void CentersTableBuilder::WriteBlock(Writer & w, std::vector<m2::PointU>::const_iterator begin,
                                     std::vector<m2::PointU>::const_iterator end) const
{
  uint64_t delta = coding::EncodePointDeltaAsUint(*begin, m_codingParams.GetBasePoint());
  WriteVarUint(w, delta);
  auto prevIt = begin;
  for (auto it = begin + 1; it != end; ++it)
  {
    delta = coding::EncodePointDeltaAsUint(*it, *prevIt);
    WriteVarUint(w, delta);
    prevIt = it;
  }
}

void CentersTableBuilder::Freeze(Writer & writer) const
{
  uint64_t const startOffset = writer.Pos();
  CHECK(coding::IsAlign8(startOffset), ());

  CentersTable::Header header;
  header.Serialize(writer);

  uint64_t bytesWritten = writer.Pos();
  coding::WritePadding(writer, bytesWritten);

  header.m_geometryParamsOffset = base::asserted_cast<uint32_t>(writer.Pos() - startOffset);
  m_codingParams.Save(writer);
  auto leftBottom = PointDToPointU(m_limitRect.LeftBottom(), kPointCoordBits);
  WriteToSink(writer, leftBottom.x);
  WriteToSink(writer, leftBottom.y);
  auto rightTop = PointDToPointU(m_limitRect.RightTop(), kPointCoordBits);
  WriteToSink(writer, rightTop.x);
  WriteToSink(writer, rightTop.y);
  header.m_geometryParamsSize =
      base::asserted_cast<uint32_t>(writer.Pos() - header.m_geometryParamsOffset - startOffset);

  bytesWritten = writer.Pos();
  coding::WritePadding(writer, bytesWritten);

  header.m_centersOffset = base::asserted_cast<uint32_t>(writer.Pos() - startOffset);
  m_builder.Freeze(writer, [&](auto & w, auto begin, auto end) { WriteBlock(w, begin, end); });
  header.m_centersSize = base::asserted_cast<uint32_t>(writer.Pos() - header.m_centersOffset - startOffset);

  auto const endOffset = writer.Pos();
  writer.Seek(startOffset);
  header.Serialize(writer);
  writer.Seek(endOffset);
}

void CentersTableBuilder::SetGeometryCodingParamsV0ForTests(serial::GeometryCodingParams const & codingParams)
{
  m_codingParams = codingParams;
}

void CentersTableBuilder::PutV0ForTests(uint32_t featureId, m2::PointD const & center)
{
  m_builder.Put(featureId, PointDToPointU(center, m_codingParams.GetCoordBits()));
}

void CentersTableBuilder::FreezeV0ForTests(Writer & writer) const
{
  m_builder.Freeze(writer, [&](auto & w, auto begin, auto end) { WriteBlock(w, begin, end); });
}
}  // namespace search
