#pragma once

#include "indexer/terrain/tile_mesh.hpp"
#include "indexer/terrain/twm_grid.hpp"
#include "indexer/terrain/twm_set.hpp"

#include "geometry/rect2d.hpp"

#include <functional>
#include <vector>

namespace terrain
{
// Serves terrain meshes from the registry. Storage owns download, replacement and deletion decisions.
class TerrainProvider : private TwmSet::Observer
{
public:
  TerrainProvider();
  ~TerrainProvider() override;

  using TerrainDeregisteredCallback = std::function<void(TwmFile const &)>;
  void SetOnTerrainDeregisteredCallback(TerrainDeregisteredCallback const & callback)
  {
    m_onTerrainDeregistered = callback;
  }

  void Clear();
  std::vector<TwmFile> GetRegisteredFiles() const;
  bool IsFileInUse(TwmFile const & file) const { return m_set.IsFileAlive(file); }

  // Extend invalidRect over the affected registered files.
  bool RegisterBlock(TwmFile const & file, m2::RectD & invalidRect);
  void DeleteBlocks(std::vector<TerrainId> const & ids, m2::RectD & invalidRect);

  // Returns true if any registered terrain block intersects the mercator rect.
  // Cheap registry lookup, safe for the UI thread.
  bool HasTerrain(m2::RectD const & rect) const { return m_set.HasBlocks(rect); }

  /// The rects of the downloaded (registered) blocks intersecting the mercator rect,
  /// e.g. for the downloaded regions highlight on the world zoom.
  void GetDownloadedRects(m2::RectD const & rect, std::vector<m2::RectD> & rects) const
  {
    m_set.GetBlockRectsByRect(rect, rects);
  }

  // Reads the merged deduplicated mesh of the features intersecting the mercator rect
  // at the geometry scale selected for the draw zoom: the single source for the
  // hillshading and the isolines of a tile (see RuleDrawer::DrawTerrain). Called from
  // the drape tile reading threads.
  void ReadMesh(m2::RectD const & rect, int zoom, TileMesh & mesh) const;

private:
  void OnTerrainDeregistered(TwmFile const & file) override;

  TerrainDeregisteredCallback m_onTerrainDeregistered;
  // Mutable: the const queries lock the readers and condemn the corrupt blocks.
  mutable TwmSet m_set;
};
}  // namespace terrain
