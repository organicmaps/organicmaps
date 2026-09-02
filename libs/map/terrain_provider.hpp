#pragma once

#include "storage/country_info_getter.hpp"

#include "indexer/terrain/tile_mesh.hpp"
#include "indexer/terrain/twm_grid.hpp"
#include "indexer/terrain/twm_set.hpp"

#include "geometry/rect2d.hpp"

#include <atomic>
#include <functional>
#include <string>
#include <string_view>

namespace terrain
{
// The provider of terrain meshes for hillshading, dynamic isolines and debug rendering:
// scans downloaded .twm files into the TwmSet registry and serves the drape tile reading threads.
// The set hands the opened readers off exclusively (concurrent queries get own values), so the FileReader caches need
// no sharing; blocks detected corrupt are condemned and never retried.
class TerrainProvider
{
public:
  // dir is the directory with the .twm files.
  explicit TerrainProvider(std::string const & dir) : m_dir(dir) {}

  // Rescans the directory: registers the files of every version folder (newest first, so
  // the TwmSet overlap rejection implements "the newest data wins, the older
  // non-overlapping blocks keep rendering"), plus the flat legacy files as the version 0.
  void Rescan();
  void Clear();

  // The currently registered files - the on-disk truth for Storage::OnTerrainScanned.
  // Queried at the publish time, not snapshotted at the scan time: a block deleted (or
  // condemned) since the scan has left the registry and must not report.
  std::vector<TwmFile> GetRegisteredFiles() const;

  // False until the first Rescan: the registry emptiness means nothing yet.
  bool IsScanned() const { return m_scanned; }

  // The downloaded block landed (see Storage terrain downloading): registers it and
  // deletes the registered OLDER blocks it intersects, even partially - the newer
  // coverage replaces them (the deregistration is delayed past the running queries).
  // Extends invalidRect over the deleted blocks, so the caller invalidates all the
  // affected tiles at once.
  void OnBlockDownloaded(std::string const & path, m2::RectD & invalidRect);

  // Deletes every registered block intersecting any of the rects, all the versions:
  // the region terrain delete from the downloader UI. Extends invalidRect the same way.
  void DeleteBlocks(std::vector<m2::RectD> const & rects, m2::RectD & invalidRect);

  // Returns true if any registered terrain block intersects the mercator rect.
  // Cheap registry lookup, safe for the UI thread.
  bool HasTerrain(m2::RectD const & rect) const { return m_set.HasBlocks(rect); }

  // True when a registered block older than the version intersects the rect: the
  // OnDiskOutOfDate terrain status source. Safe for the UI thread.
  bool HasOlderTerrain(m2::RectD const & rect, int64_t version) const { return m_set.HasOlderBlocks(rect, version); }

  // Reads the merged deduplicated mesh of the features intersecting the mercator rect
  // at the geometry scale selected for the draw zoom: the single source for the
  // hillshading and the isolines of a tile (see RuleDrawer::DrawTerrain). Called from
  // the drape tile reading threads.
  void ReadMesh(m2::RectD const & rect, int zoom, TileMesh & mesh) const;

private:
  void DeleteBlocksImpl(std::vector<m2::RectD> const & rects, std::function<bool(TwmInfo const &)> const & pred,
                        m2::RectD & invalidRect);

  std::string m_dir;
  std::atomic<bool> m_scanned{false};
  // Mutable: the const queries lock the readers and condemn the corrupt blocks.
  mutable TwmSet m_set;
};

// A tile draws its terrain where the map does: the blocks are shared by the neighbor
// regions, so a tile whose centre lies in a region that is not downloaded shows the
// "Download" call to action instead of the neighbor's relief, while the ocean (no region)
// keeps its bathymetry. One region lookup per tile read; the region reader and the loaded
// check are locked on their own, so the tile reading threads may call it.
using IsCountryLoadedFn = std::function<bool(std::string_view)>;
bool IsTerrainDrawableAt(storage::CountryInfoGetter const & infoGetter, IsCountryLoadedFn const & isLoaded,
                         m2::PointD const & tileCenter);
}  // namespace terrain
