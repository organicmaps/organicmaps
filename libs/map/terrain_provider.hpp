#pragma once

#include "indexer/terrain/terrain_data.hpp"
#include "indexer/terrain/twm_grid.hpp"
#include "indexer/terrain/twm_set.hpp"

#include "geometry/rect2d.hpp"

#include "base/cancellable.hpp"

#include <functional>
#include <mutex>
#include <optional>
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
  /// Drops the idle cached readers (e.g. on a memory warning).
  void ClearCache() { m_set.ClearCache(); }
  std::vector<TwmFile> GetRegisteredFiles() const;
  bool IsFileInUse(TwmFile const & file) const { return m_set.IsFileAlive(file); }

  // If requested, extend invalidRect over the affected registered files.
  bool RegisterBlock(TwmFile const & file, m2::RectD * invalidRect = nullptr);
  void DeleteBlocks(std::vector<TerrainId> const & ids, m2::RectD & invalidRect);

  // Returns true if any registered terrain block intersects the mercator rect.
  // Cheap registry lookup, safe for the UI thread.
  bool HasTerrain(m2::RectD const & rect) const { return m_set.HasBlocks(rect); }

  // Elevation in meters from the finest downloaded terrain surface, if present.
  std::optional<double> GetElevation(m2::PointD const & point) const;

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
  void ReadTerrain(m2::RectD const & rect, int zoom, TerrainData & data,
                   base::SharedCancellable const & isCancellable = {}) const;

private:
  void AddInvalidationRect(m2::RectD rect, m2::RectD & invalidRect) const;
  void OnTerrainDeregistered(TwmFile const & file) override;

  TerrainDeregisteredCallback m_onTerrainDeregistered;
  TwmSet m_set;
  // The largest halo of the reads so far bounds the area around a block that the cached tiles read.
  mutable std::mutex m_readHaloMutex;
  mutable m2::PointD m_maxReadHalo{0, 0};
};
}  // namespace terrain
