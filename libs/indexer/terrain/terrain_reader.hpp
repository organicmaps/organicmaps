#pragma once

#include "indexer/interval_index.hpp"
#include "indexer/terrain/terrain_serdes.hpp"

#include "coding/files_container.hpp"

#include "geometry/rect2d.hpp"

#include <memory>

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

  // Decodes every feature intersecting the mercator rect at the given geometry scale
  // index, in the file order, straight into the tile mesh (see DecodeChains).
  void ReadMesh(m2::RectD const & rect, size_t geomIndex, TileMesh & mesh) const;

private:
  FilesContainerR m_container;
  TwmHeader m_header;
  std::unique_ptr<IntervalIndex<ModelReaderPtr, uint32_t>> m_index;
};
}  // namespace terrain
