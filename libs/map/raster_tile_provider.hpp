#pragma once

#include "drape_frontend/tile_key.hpp"

#include "drape/drape_global.hpp"

#include "geometry/rect2d.hpp"

#include <array>
#include <functional>
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// POC raster background-tile provider.
//
// Downloads standard web-mercator XYZ raster tiles (e.g. https://host/{z}/{x}/{y}.png),
// caches the encoded files on disk, decodes them to RGBA8 and feeds them into the Drape
// tile-background rendering API. Integration point: Framework::CreateDrapeEngine's
// tileBackgroundReadFn (see libs/map/framework.cpp).
//
// Encoded tiles are cached on disk under an LRU size cap (Params::m_maxCacheBytes): the
// least-recently-used tiles are evicted once the total is exceeded. Content is delivered through a
// callback that the caller wires to df::DrapeEngine::AddTileBackgroundImage + SetTileBackgroundData.

// Debug visualization: renders status placeholders (DOWNLOADING / NOT FOUND / ERROR) into tiles.
// Binding a placeholder moves the tile out of the renderer's awaiting set, which conflicts with
// the m_awaitingTiles bookkeeping (the real result must win a second bind; failed tiles are not
// re-requested), so it is disabled by default.
// #define ENABLE_STATUS_PLACEHOLDERS
class RasterTileProvider
{
public:
  struct Params
  {
    // URL template that must contain the {z}, {x}, {y} placeholders.
    std::string m_urlTemplate;
    // Sub-directory under Platform::WritableDir() used for the on-disk tile cache.
    std::string m_cacheSubdir = "bg_tiles_poc";
    // Minimum source (web-mercator) zoom to request. Tiles below it are skipped (low zooms are
    // often near-empty / look bad for this endpoint).
    int m_minZoom = 0;
    // Maximum source (web-mercator) zoom available on the server. Deeper OM tiles reuse a
    // sub-rect of the ancestor tile at this zoom.
    int m_maxZoom = 19;
    // Optional WGS84 coverage box; tiles fully outside it are skipped (avoids 404 spam).
    double m_minLat = -90.0;
    double m_maxLat = 90.0;
    double m_minLon = -180.0;
    double m_maxLon = 180.0;
    // Upper bound on the on-disk tile cache. Once exceeded, least-recently-used tiles are evicted.
    uint64_t m_maxCacheBytes = 100ull * 1024 * 1024;  // 100 MB
  };

  // Delivers decoded RGBA8 pixels for an OM tile. Invoked on a background (Network) thread.
  // Mirrors the df::DrapeEngine::AddTileBackgroundImage + SetTileBackgroundData call pair:
  //   AddTileBackgroundImage(imageUid, width, height, RGBA8, mode, std::move(rgba));
  //   SetTileBackgroundData(tileKey, imageUid, rect);
  using TReadyFn =
      std::function<void(df::TileKey const & tileKey, dp::BackgroundMode mode, std::string const & imageUid,
                         uint32_t width, uint32_t height, m2::RectF const & rect, std::vector<uint8_t> && rgba)>;

  RasterTileProvider(Params params, TReadyFn onReady);

  // Called on the render thread for every newly visible tile. Non-blocking: schedules the
  // fetch/decode on the Network thread pool.
  bool RequestTile(df::TileKey const & tileKey, dp::BackgroundMode mode);

  // Called on the render thread when a tile scrolls out of view; drops a not-yet-delivered
  // request so it is neither bound nor uploaded once it finishes.
  void CancelTile(df::TileKey const & tileKey, dp::BackgroundMode mode);

private:
  // Source web-mercator XYZ tile plus the UV sub-rect within it that the given OM tile maps to.
  struct SourceTile
  {
    int m_z = 0;
    int m_x = 0;
    int m_y = 0;
    m2::RectF m_rect{0.0f, 0.0f, 1.0f, 1.0f};
    bool m_valid = false;
  };

#ifdef ENABLE_STATUS_PLACEHOLDERS
  // Debug placeholder shown in place of a tile while it is being fetched or when it is missing.
  enum class Status
  {
    Downloading = 0,
    NotFound,
    Error,
    Count
  };

  struct Placeholder
  {
    std::string m_uid;
    std::vector<uint8_t> m_rgba;
    uint32_t m_size = 0;
  };
#endif

  SourceTile ToSourceTile(df::TileKey const & tileKey) const;

  // Starts a non-blocking HTTP request; its completion handler decodes, caches and delivers the tile.
  void StartDownload(df::TileKey const & tileKey, dp::BackgroundMode mode, std::string const & url,
                     std::string const & uid, std::string const & fileName, m2::RectF const & rect);

#ifdef ENABLE_STATUS_PLACEHOLDERS
  // Delivers a debug status placeholder image for the tile through m_onReady.
  void DeliverPlaceholder(df::TileKey const & tileKey, dp::BackgroundMode mode, Status status);
#endif

  bool IsActive(df::TileKey const & tileKey) const;
  // Removes tileKey from the in-flight set. Returns true if it was still present
  // (i.e. the request was not cancelled meanwhile).
  bool DropActive(df::TileKey const & tileKey);

  // Scans the cache dir (posted to the File thread from the constructor, ahead of any cache read)
  // and seeds the LRU order by file time.
  void InitDiskCache();
  // Marks a cached tile (by file name) as most-recently-used (on a cache hit).
  void TouchCacheEntry(std::string const & name);
  // Registers a freshly-written tile (by file name), then evicts least-recently-used files over the cap.
  void AddCacheEntry(std::string const & name, uint64_t size);
  // Evicts oldest entries until the total is within the cap. Caller must hold m_cacheMutex.
  void EvictDiskCacheLocked();

  Params const m_params;
  TReadyFn const m_onReady;
  std::string m_cacheDir;
#ifdef ENABLE_STATUS_PLACEHOLDERS
  std::array<Placeholder, static_cast<size_t>(Status::Count)> m_placeholders;
#endif

  mutable std::mutex m_activeMutex;
  std::unordered_set<df::TileKey> m_active;

  // On-disk cache index, keyed by tile file name. m_lru orders names front (LRU) -> back (MRU);
  // m_cacheBytes is the tracked total. Guarded by m_cacheMutex.
  std::mutex m_cacheMutex;
  std::list<std::string> m_lru;

  // One on-disk cache file: its byte size and its node in the LRU list (front == least recent).
  struct CacheEntry
  {
    uint64_t m_size = 0;
    std::list<std::string>::iterator m_lruIt;
  };
  std::unordered_map<std::string, CacheEntry> m_cacheIndex;

  uint64_t m_cacheBytes = 0;
};
