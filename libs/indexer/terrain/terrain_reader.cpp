#include "indexer/terrain/terrain_reader.hpp"

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
}

void Reader::ReadMesh(m2::RectD const & rect, size_t geomIndex, TileMesh & mesh) const
{
  auto const intervals = covering::CoverViewportAndAppendLowerLevels<RectId::DEPTH_LEVELS>(rect, kCellDepth);
  ReadTin(rect, intervals, geomIndex, mesh);
}

void Reader::ReadTin(m2::RectD const & rect, covering::Intervals const & intervals, size_t geomIndex, TileMesh & mesh,
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

  // The decoder working vectors, grown once per block read instead of per feature.
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

  if (isCancellable.IsCancelled())
    return;

  covering::Intervals intervals;
  {
    ScopedReadTimer const timer(stats ? &stats->m_tinIndexSeconds : nullptr);
    intervals = covering::CoverViewportAndAppendLowerLevels<RectId::DEPTH_LEVELS>(rect, kCellDepth);
  }
  ReadTin(rect, intervals, geomIndex, data.m_mesh, isCancellable, stats);
}
}  // namespace terrain
