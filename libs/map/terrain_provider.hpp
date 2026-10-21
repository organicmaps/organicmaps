#pragma once

#include "indexer/terrain/terrain_data.hpp"
#include "indexer/terrain/twm_grid.hpp"
#include "indexer/terrain/twm_set.hpp"

#include "geometry/rect2d.hpp"

#include "base/cancellable.hpp"

#include <vector>

namespace terrain
{
// Serves terrain meshes from the registry of the installed terrain blocks.
class TerrainProvider
{
public:
  void Clear();
  /// Drops the idle cached readers (e.g. on a memory warning).
  void ClearCache() { m_set.ClearCache(); }
  std::vector<TwmFile> GetRegisteredFiles() const;

  bool RegisterBlock(TwmFile const & file);

  // Returns true if any registered terrain block intersects the mercator rect.
  // Cheap registry lookup, safe for the UI thread.
  bool HasTerrain(m2::RectD const & rect) const { return m_set.HasBlocks(rect); }

  // Reads the merged deduplicated mesh of the features intersecting the mercator rect
  // at the geometry scale selected for the draw zoom: the single source for the
  // hillshading and the isolines of a tile (see RuleDrawer::DrawTerrain). Called from
  // the drape tile reading threads.
  void ReadTerrain(m2::RectD const & rect, int zoom, TerrainData & data,
                   base::SharedCancellable const & isCancellable = {}) const;

private:
  TwmSet m_set;
};
}  // namespace terrain
