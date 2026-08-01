#pragma once

#include "indexer/terrain/tile_mesh.hpp"
#include "indexer/terrain/twm_set.hpp"

#include "geometry/rect2d.hpp"

#include <atomic>
#include <functional>
#include <string>

namespace terrain
{
// The provider of the dynamic isolines over the downloaded .twm terrain files:
// scans the directory into the TwmSet registry and serves the queries from the drape tile reading threads.
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
  void DeleteBlocksImpl(std::vector<m2::RectD> const & rects, std::function<bool(TwmInfo const &)> const & pred,
                        m2::RectD & invalidRect);

  std::string m_dir;
  std::atomic<bool> m_scanned{false};
  // Mutable: the const queries lock the readers and condemn the corrupt blocks.
  mutable TwmSet m_set;
};
}  // namespace terrain
