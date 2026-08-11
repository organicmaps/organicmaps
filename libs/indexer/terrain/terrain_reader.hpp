#pragma once

#include "indexer/interval_index.hpp"
#include "indexer/terrain/terrain_serdes.hpp"

#include "coding/files_container.hpp"

#include "geometry/rect2d.hpp"

#include <memory>
#include <vector>

namespace terrain
{
// Reads TrianglesFeatures from a .twm file.
// Corrupt or unsupported data throws TwmException or Reader::Exception (both RootException),
// the caller is expected to catch them and drop the file.
class Reader
{
public:
  explicit Reader(FilesContainerR const & container);

  TwmHeader const & GetHeader() const { return m_header; }
  // The lattice tables of the file meshes, indexed by FeatureRecord::m_meshIdx.
  std::vector<MeshGrid> const & GetGrids() const { return m_grids; }

  // Decodes every feature intersecting the mercator rect at the given geometry scale
  // index, in the file order, straight into the tile mesh (see DecodeChains).
  void ReadMesh(m2::RectD const & rect, size_t geomIndex, TileMesh & mesh) const;

private:
  FilesContainerR m_container;
  TwmHeader m_header;
  // Loaded upfront: small (0.03-0.34% of the file) and every feature needs its mesh tables.
  std::vector<MeshGrid> m_grids;
  // The entropy tables of the geometry sections, one per geometry scale, tiny and needed by
  // every geometry decode of their scale.
  std::vector<GeometryTables> m_tables;
  std::unique_ptr<IntervalIndex<ModelReaderPtr, uint32_t>> m_index;
};
}  // namespace terrain
