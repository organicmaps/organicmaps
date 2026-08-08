#include "indexer/terrain/terrain_reader.hpp"

#include "indexer/feature_covering.hpp"

#include <set>

namespace terrain
{
Reader::Reader(FilesContainerR const & container) : m_container(container)
{
  {
    ReaderSource<ModelReaderPtr> src(m_container.GetReader(kHeaderTag));
    m_header.Deserialize(src);
  }
  {
    ReaderSource<ModelReaderPtr> src(m_container.GetReader(kGridTag));
    DeserializeGrid(src, m_header, m_grids);
  }
  {
    ReaderSource<ModelReaderPtr> src(m_container.GetReader(kFreqTag));
    DeserializeFreqs(src, m_header, m_tables);
  }

  auto const indexReader = m_container.GetReader(kIndexTag);
  // IntervalIndex CHECKs (aborts) on a version mismatch, keep the corrupt data catchable.
  {
    ReaderSource<ModelReaderPtr> src(indexReader);
    if (src.Size() < sizeof(IntervalIndexBase::Header) ||
        ReadPrimitiveFromSource<uint8_t>(src) != IntervalIndexBase::kVersion)
    {
      MYTHROW(TwmException, ("Unsupported index format"));
    }
  }
  m_index = std::make_unique<IntervalIndex<ModelReaderPtr, uint32_t>>(indexReader);
}

void Reader::ReadMesh(m2::RectD const & rect, size_t geomIndex, TileMesh & mesh) const
{
  ASSERT_LESS(geomIndex, m_header.m_geometries.size(), ());
  ASSERT_EQUAL(m_tables.size(), m_header.m_geometries.size(), ());
  // The mesh dedup keys are raw quantized points: the quantization must be uniform.
  ASSERT_EQUAL(mesh.GetCoordBits(), m_header.m_coordBits, ());
  // The record mesh index is validated against the same count on the record deserialization.
  ASSERT_EQUAL(m_grids.size(), m_header.m_meshCount, ());

  // Sorted and deduplicated: read the feature records in the file order.
  auto const intervals = covering::CoverViewportAndAppendLowerLevels<RectId::DEPTH_LEVELS>(rect, kCellDepth);
  std::set<uint32_t> offsets;
  for (auto const & interval : intervals)
  {
    m_index->ForEach([&offsets](uint64_t /* key */, uint32_t offset) { offsets.insert(offset); }, interval.first,
                     interval.second);
  }
  if (offsets.empty())
    return;

  auto const features = m_container.GetReader(kFeaturesTag);
  auto const geometry = m_container.GetReader(GetGeometryTag(geomIndex));
  uint64_t const featuresSize = features.Size();
  uint64_t const geometrySize = geometry.Size();
  // The decoder working vectors, grown once for the whole tile instead of per feature.
  GeometryScratch scratch;
  for (uint32_t const offset : offsets)
  {
    // Guards the unsigned size subtraction below. A corrupt index must stay catchable.
    if (offset >= featuresSize)
      MYTHROW(TwmException, ("Feature offset out of the section", offset, featuresSize));
    ReaderSource<ModelReaderPtr> src(features.SubReader(offset, featuresSize - offset));
    FeatureRecord record;
    DeserializeFeatureRecord(src, m_header, record);

    if (!rect.IsIntersect(record.GetRect(m_header.m_coordBits)))
      continue;

    uint64_t const geomOffset = record.m_geomOffsets[geomIndex];
    if (geomOffset >= geometrySize)
      MYTHROW(TwmException, ("Geometry offset out of the section", geomOffset, geometrySize));
    ReaderSource<ModelReaderPtr> geomSrc(geometry.SubReader(geomOffset, geometrySize - geomOffset));
    DeserializeFeatureGeometry(geomSrc, m_grids[record.m_meshIdx], m_tables[geomIndex], record, scratch, mesh);
  }
}
}  // namespace terrain
