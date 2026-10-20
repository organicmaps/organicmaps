#include "indexer/terrain/terrain_reader.hpp"

#include "indexer/feature_covering.hpp"

#include "geometry/mercator.hpp"

#include "base/buffer_vector.hpp"
#include "base/stl_helpers.hpp"
#include "base/timer.hpp"

#include <algorithm>

namespace terrain
{
namespace
{
struct UniqueOffsets
{
  void operator()(uint64_t /* key */, uint32_t offset) { m_offsets.push_back(offset); }
  void Sort() { base::SortUnique(m_offsets); }

  buffer_vector<uint32_t, 64> m_offsets;
};

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

Reader::Reader(std::string const & filePath) : m_container(filePath)
{
  {
    ReaderSource src(m_container.GetReader(kHeaderTag));
    m_header.Deserialize(src);
  }
  {
    ReaderSource src(m_container.GetReader(kGridTag));
    DeserializeGrid(src, m_header, m_grids);
  }
  {
    ReaderSource src(m_container.GetReader(kFreqTag));
    DeserializeFreqs(src, m_header, m_tables);
  }

  m_index.emplace(m_container.GetReader(kIndexTag));
  if (m_header.m_rasterPatchCount != 0)
  {
    m_rasterIndex.emplace(m_container.GetReader(kRasterIndexTag));
    uint32_t dx = 0, dy = 0;
    // All lattice tables give a conservative cell margin without reading raster records.
    for (auto const & grid : m_grids)
    {
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

std::optional<double> Reader::GetElevation(m2::PointD const & point) const
{
  if (!m_header.GetLimitRect().IsPointInside(point))
    return std::nullopt;

  // A positive query extent also covers both sides of index-cell boundaries.
  double const unit = mercator::Bounds::kRangeX / ((uint64_t{1} << m_header.m_coordBits) - 1);
  m2::RectD query(point, point);
  query.Inflate(unit, unit);
  query.Intersect(mercator::Bounds::FullRect());
  TerrainData data(m_header.m_coordBits);
  ReadTerrain(query, m_header.m_geometries.size() - 1, data);
  return data.GetElevation(point);
}

void Reader::ReadTin(m2::RectD const & rect, size_t geomIndex, TileMesh & mesh,
                     base::SharedCancellable const & isCancellable, ReadStats * stats) const
{
  ASSERT_LESS(geomIndex, m_header.m_geometries.size(), ());
  ASSERT_EQUAL(m_tables.size(), m_header.m_geometries.size(), ());
  // The mesh dedup keys are raw quantized points: the quantization must be uniform.
  ASSERT_EQUAL(mesh.GetCoordBits(), m_header.m_coordBits, ());
  // The record mesh index is validated against the same count on the record deserialization.
  ASSERT_EQUAL(m_grids.size(), m_header.m_meshCount, ());

  // Sorted and deduplicated: read the feature records in the file order.
  UniqueOffsets uniqueOffsets;
  {
    ScopedReadTimer const timer(stats ? &stats->m_tinIndexSeconds : nullptr);
    auto const intervals = covering::CoverViewportAndAppendLowerLevels<RectId::DEPTH_LEVELS>(rect, kCellDepth);
    for (auto const & interval : intervals)
      m_index->ForEach(uniqueOffsets, interval.first, interval.second);
    uniqueOffsets.Sort();
  }

  auto const & offsets = uniqueOffsets.m_offsets;
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
    if (isCancellable.IsCancelled())
      return;

    FeatureRecord record;
    {
      ScopedReadTimer const timer(stats ? &stats->m_tinRecordsSeconds : nullptr);
      CHECK_LESS(offset, featuresSize, ());
      ReaderSource src(features.SubReader(offset, featuresSize - offset));
      DeserializeFeatureRecord(src, m_header, record);
      if (!rect.IsIntersect(record.GetRect(m_header.m_coordBits)))
        continue;
    }

    if (stats)
      ++stats->m_tinFeatures;

    ScopedReadTimer const timer(stats ? &stats->m_tinGeometrySeconds : nullptr);
    uint64_t const geomOffset = record.m_geomOffsets[geomIndex];
    CHECK_LESS(geomOffset, geometrySize, ());
    ReaderSource geomSrc(geometry.SubReader(geomOffset, geometrySize - geomOffset));
    DeserializeFeatureGeometry(geomSrc, m_grids[record.m_meshIdx], m_tables[geomIndex], record, scratch, mesh);
  }
}

void Reader::ReadTerrain(m2::RectD const & rect, size_t geomIndex, TerrainData & data,
                         base::SharedCancellable const & isCancellable, ReadStats * stats) const
{
  ASSERT_EQUAL(data.m_mesh.GetCoordBits(), m_header.m_coordBits, ());

  TerrainData decoded(m_header.m_coordBits);
  ReadTin(rect, geomIndex, decoded.m_mesh, isCancellable, stats);

  if (m_header.HasRaster(geomIndex))
  {
    UniqueOffsets uniqueOffsets;
    {
      ScopedReadTimer const timer(stats ? &stats->m_rasterIndexSeconds : nullptr);
      auto const intervals = covering::CoverViewportAndAppendLowerLevels<RectId::DEPTH_LEVELS>(rect, kCellDepth);
      for (auto const & interval : intervals)
        m_rasterIndex->ForEach(uniqueOffsets, interval.first, interval.second);
      uniqueOffsets.Sort();
    }

    auto const & offsets = uniqueOffsets.m_offsets;
    if (stats)
      stats->m_rasterCandidates += offsets.size();

    // Should continue with data.Append if raster offsets is empty.
    if (!offsets.empty())
    {
      auto const features = m_container.GetReader(kRasterFeaturesTag);
      auto const payload = m_container.GetReader(kRasterDataTag);

      buffer_vector<uint8_t, 8 * 1024> bytes;
      for (uint32_t offset : offsets)
      {
        if (isCancellable.IsCancelled())
          return;

        RasterRecord record;
        {
          ScopedReadTimer const timer(stats ? &stats->m_rasterRecordsSeconds : nullptr);
          CHECK_LESS(offset, features.Size(), ());
          ReaderSource src(features.SubReader(offset, features.Size() - offset));
          record = DeserializeRasterRecord(src);
          CHECK_LESS(record.m_gridIndex, m_grids.size(), ());
          record.Validate(m_header, m_grids[record.m_gridIndex]);
          if (!rect.IsIntersect(record.GetRect(m_grids[record.m_gridIndex], m_header.m_coordBits)))
            continue;
        }

        if (stats)
        {
          ++stats->m_rasterPatches;
          stats->m_rasterBytes += record.m_dataLength;
        }

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
          auto const gridRect = record.GetGridRect(grid);
          RasterPatch patch;
          patch.m_coordBits = m_header.m_coordBits;
          patch.m_columns.assign(grid.m_colX.begin() + gridRect.minX(), grid.m_colX.begin() + gridRect.maxX() + 1);
          patch.m_rows.assign(grid.m_rowY.begin() + gridRect.minY(), grid.m_rowY.begin() + gridRect.maxY() + 1);
          patch.m_heights = DecodeRasterPayload(record, bytes, gridRect.SizeX(), gridRect.SizeY());
          decoded.m_rasterPatches.push_back(std::move(patch));
        }
      }
    }
  }

  if (isCancellable.IsCancelled())
    return;

  // Cancellation leaves data intact.
  ScopedReadTimer const timer(stats ? &stats->m_appendSeconds : nullptr);
  data.Append(std::move(decoded));
}
}  // namespace terrain
