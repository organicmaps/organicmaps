#include "indexer/terrain/terrain_reader.hpp"

#include "indexer/feature_covering.hpp"

#include "base/timer.hpp"

#include <algorithm>
#include <set>

namespace terrain
{
namespace
{
class ScopedReadTimer
{
public:
  explicit ScopedReadTimer(double * seconds) : m_seconds(seconds), m_timer(false)
  {
    if (m_seconds)
      m_timer.Reset();
  }

  ~ScopedReadTimer()
  {
    if (m_seconds)
      *m_seconds += m_timer.ElapsedSeconds();
  }

private:
  double * m_seconds;
  base::Timer m_timer;
};
}  // namespace

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

  auto const readIndex = [&](char const * tag)
  {
    auto const indexReader = m_container.GetReader(tag);
    // Reject unsupported interval-index headers before construction.
    {
      ReaderSource<ModelReaderPtr> src(indexReader);
      if (src.Size() < sizeof(IntervalIndexBase::Header) ||
          ReadPrimitiveFromSource<uint8_t>(src) != IntervalIndexBase::kVersion)
      {
        MYTHROW(TwmException, ("Unsupported index format"));
      }
    }
    return std::make_unique<IntervalIndex<ModelReaderPtr, uint32_t>>(indexReader);
  };
  m_index = readIndex(kIndexTag);
  if (m_header.m_rasterPatchCount != 0)
  {
    m_rasterIndex = readIndex(kRasterIndexTag);
    uint32_t dx = 0, dy = 0;
    std::set<uint32_t> rasterGrids;
    ReaderSource<ModelReaderPtr> records(m_container.GetReader(kRasterFeaturesTag));
    for (uint32_t i = 0; i < m_header.m_rasterPatchCount; ++i)
    {
      auto const record = DeserializeRasterRecord(records);
      record.Validate(m_header, m_grids);
      rasterGrids.insert(record.m_gridIndex);
    }
    ASSERT_EQUAL(records.Size(), 0, ());
    for (auto const index : rasterGrids)
    {
      auto const & grid = m_grids[index];
      for (size_t i = 1; i < grid.m_colX.size(); ++i)
        dx = std::max(dx, grid.m_colX[i] - grid.m_colX[i - 1]);
      for (size_t i = 1; i < grid.m_rowY.size(); ++i)
        dy = std::max(dy, grid.m_rowY[i] - grid.m_rowY[i - 1]);
    }
    m_rasterHalo = PointUToPointD({dx, dy}, m_header.m_coordBits) - PointUToPointD({0, 0}, m_header.m_coordBits);
  }
}

void Reader::ReadMesh(m2::RectD const & rect, size_t geomIndex, TileMesh & mesh) const
{
  ASSERT(!m_header.HasRaster(geomIndex), ("ReadMesh cannot read a raster LOD; use ReadTerrain"));
  ReadTin(rect, geomIndex, mesh);
}

void Reader::ReadTin(m2::RectD const & rect, size_t geomIndex, TileMesh & mesh,
                     std::function<bool()> const & isCancelled, ReadStats * stats) const
{
  ASSERT_LESS(geomIndex, m_header.m_geometries.size(), ());
  ASSERT_EQUAL(m_tables.size(), m_header.m_geometries.size(), ());
  // The mesh dedup keys are raw quantized points: the quantization must be uniform.
  ASSERT_EQUAL(mesh.GetCoordBits(), m_header.m_coordBits, ());
  // The record mesh index is validated against the same count on the record deserialization.
  ASSERT_EQUAL(m_grids.size(), m_header.m_meshCount, ());

  // Sorted and deduplicated: read the feature records in the file order.
  std::set<uint32_t> offsets;
  {
    ScopedReadTimer const timer(stats ? &stats->m_tinIndexSeconds : nullptr);
    auto const intervals = covering::CoverViewportAndAppendLowerLevels<RectId::DEPTH_LEVELS>(rect, kCellDepth);
    for (auto const & interval : intervals)
    {
      m_index->ForEach([&offsets](uint64_t /* key */, uint32_t offset) { offsets.insert(offset); }, interval.first,
                       interval.second);
    }
  }
  if (stats)
    stats->m_tinCandidates += offsets.size();
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
    if (isCancelled && isCancelled())
      return;
    FeatureRecord record;
    bool intersects;
    {
      ScopedReadTimer const timer(stats ? &stats->m_tinRecordsSeconds : nullptr);
      CHECK_LESS(offset, featuresSize, ());
      ReaderSource<ModelReaderPtr> src(features.SubReader(offset, featuresSize - offset));
      DeserializeFeatureRecord(src, m_header, record);
      intersects = rect.IsIntersect(record.GetRect(m_header.m_coordBits));
    }
    if (!intersects)
      continue;
    if (stats)
      ++stats->m_tinFeatures;
    ScopedReadTimer const timer(stats ? &stats->m_tinGeometrySeconds : nullptr);
    uint64_t const geomOffset = record.m_geomOffsets[geomIndex];
    CHECK_LESS(geomOffset, geometrySize, ());
    ReaderSource<ModelReaderPtr> geomSrc(geometry.SubReader(geomOffset, geometrySize - geomOffset));
    DeserializeFeatureGeometry(geomSrc, m_grids[record.m_meshIdx], m_tables[geomIndex], record, scratch, mesh);
  }
}

void Reader::ReadTerrain(m2::RectD const & rect, size_t geomIndex, TerrainData & data,
                         std::function<bool()> const & isCancelled, ReadStats * stats) const
{
  ASSERT_EQUAL(data.m_mesh.GetCoordBits(), m_header.m_coordBits, ());
  TerrainData decoded(m_header.m_coordBits);
  ReadTin(rect, geomIndex, decoded.m_mesh, isCancelled, stats);
  if (m_header.HasRaster(geomIndex))
  {
    std::set<uint32_t> offsets;
    {
      ScopedReadTimer const timer(stats ? &stats->m_rasterIndexSeconds : nullptr);
      auto const intervals = covering::CoverViewportAndAppendLowerLevels<RectId::DEPTH_LEVELS>(rect, kCellDepth);
      for (auto const & interval : intervals)
        m_rasterIndex->ForEach([&](uint64_t, uint32_t offset) { offsets.insert(offset); }, interval.first,
                               interval.second);
    }
    if (stats)
      stats->m_rasterCandidates += offsets.size();
    auto const features = m_container.GetReader(kRasterFeaturesTag);
    auto const payload = m_container.GetReader(kRasterDataTag);
    for (uint32_t offset : offsets)
    {
      if (isCancelled && isCancelled())
        return;
      RasterRecord record;
      bool intersects;
      {
        ScopedReadTimer const timer(stats ? &stats->m_rasterRecordsSeconds : nullptr);
        CHECK_LESS(offset, features.Size(), ());
        ReaderSource<ModelReaderPtr> src(features.SubReader(offset, features.Size() - offset));
        record = DeserializeRasterRecord(src);
        record.Validate(m_header, m_grids);
        intersects = rect.IsIntersect(record.GetRect(m_grids[record.m_gridIndex], m_header.m_coordBits));
      }
      if (!intersects)
        continue;
      if (stats)
      {
        ++stats->m_rasterPatches;
        stats->m_rasterBytes += record.m_dataLength;
      }
      std::vector<uint8_t> bytes;
      {
        ScopedReadTimer const timer(stats ? &stats->m_rasterReadSeconds : nullptr);
        CHECK_LESS_OR_EQUAL(record.m_dataOffset, payload.Size(), ());
        CHECK_LESS_OR_EQUAL(record.m_dataLength, payload.Size() - record.m_dataOffset, ());
        bytes.resize(record.m_dataLength);
        payload.Read(record.m_dataOffset, bytes.data(), bytes.size());
      }
      {
        ScopedReadTimer const timer(stats ? &stats->m_rasterDecodeSeconds : nullptr);
        auto const & grid = m_grids[record.m_gridIndex];
        RasterPatch patch;
        patch.m_coordBits = m_header.m_coordBits;
        patch.m_columns.assign(grid.m_colX.begin() + record.m_firstColumn,
                               grid.m_colX.begin() + record.m_firstColumn + record.m_cellsX + 1);
        patch.m_rows.assign(grid.m_rowY.begin() + record.m_firstRow,
                            grid.m_rowY.begin() + record.m_firstRow + record.m_cellsY + 1);
        patch.m_heights = DecodeRasterPayload(record, bytes);
        decoded.m_rasterPatches.push_back(std::move(patch));
      }
    }
  }
  if (isCancelled && isCancelled())
    return;
  // Cancellation leaves data intact.
  ScopedReadTimer const timer(stats ? &stats->m_appendSeconds : nullptr);
  data.Append(std::move(decoded));
}
}  // namespace terrain
