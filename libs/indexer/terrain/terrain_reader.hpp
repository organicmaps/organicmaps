#pragma once

#include "indexer/feature_covering.hpp"
#include "indexer/interval_index.hpp"
#include "indexer/terrain/terrain_data.hpp"
#include "indexer/terrain/terrain_serdes.hpp"

#include "coding/files_container.hpp"

#include "geometry/rect2d.hpp"

#include "base/cancellable.hpp"

#include <optional>
#include <string>
#include <vector>

namespace terrain
{
// Optional accumulated timings for query profiling. Geometry timing includes stream reads
// and mesh construction; raster decoding includes axis copies and border restoration.
struct ReadStats
{
  double m_tinIndexSeconds = 0;
  double m_tinRecordsSeconds = 0;
  double m_tinGeometrySeconds = 0;
  double m_rasterIndexSeconds = 0;
  double m_rasterRecordsSeconds = 0;
  double m_rasterReadSeconds = 0;
  double m_rasterDecodeSeconds = 0;
  double m_appendSeconds = 0;
  uint64_t m_tinCandidates = 0;
  uint64_t m_tinFeatures = 0;
  uint64_t m_rasterCandidates = 0;
  uint64_t m_rasterPatches = 0;
  uint64_t m_rasterBytes = 0;
};

// Reads TrianglesFeatures from a .twm file.
// Unsupported headers throw TwmException. Tile data is trusted after download verification;
// invariants are asserted in Debug. Reader::Exception may report I/O failures.
class Reader
{
public:
  explicit Reader(std::string const & filePath);

  TwmHeader const & GetHeader() const { return m_header; }

  // Decodes every feature intersecting the mercator rect at the given geometry scale
  // index, in the file order, straight into the tile mesh (see DecodeChains).
  // Use ReadTerrain for a hybrid raster LOD.
  void ReadMesh(m2::RectD const & rect, size_t geomIndex, TileMesh & mesh) const;
  // Appends to data directly; cancellation or an I/O failure may leave a partial result.
  void ReadTerrain(m2::RectD const & rect, size_t geomIndex, TerrainData & data,
                   base::SharedCancellable const & isCancellable = {}, ReadStats * stats = nullptr) const;
  m2::PointD GetRasterHalo() const { return m_rasterHalo; }

private:
  void ReadTin(m2::RectD const & rect, covering::Intervals const & intervals, size_t geomIndex, TileMesh & mesh,
               base::SharedCancellable const & isCancellable = {}, ReadStats * stats = nullptr) const;
  FilesContainerR m_container;
  TwmHeader m_header;
  // Loaded upfront: small (0.03-0.34% of the file) and every feature needs its mesh tables.
  std::vector<MeshGrid> m_grids;
  // The entropy tables of the geometry sections, one per geometry scale, tiny and needed by
  // every geometry decode of their scale.
  std::vector<GeometryTables> m_tables;
  std::optional<IntervalIndex<ModelReaderPtr, uint32_t>> m_index;
  std::optional<IntervalIndex<ModelReaderPtr, uint32_t>> m_rasterIndex;
  m2::PointD m_rasterHalo{0, 0};
};
}  // namespace terrain
