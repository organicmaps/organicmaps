#pragma once

#include "drape_frontend/tile_key.hpp"

#include "drape/drape_global.hpp"

#include "platform/http_client.hpp"

#include "geometry/rect2d.hpp"

#include <functional>
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// Downloads XYZ raster tiles, caches encoded PNG/JPEG files under an LRU size cap,
// and delivers RGBA8 data to the tile-background renderer.
class RasterTileProvider
{
public:
  struct Params
  {
    // URL template that must contain the {z}, {x}, {y} placeholders.
    std::string m_urlTemplate;
    // Sub-directory under Platform::WritableDir() used for the on-disk tile cache.
    std::string m_cacheSubdir = "bg_tiles";
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

  // Delivers decoded RGBA8 pixels for an OM tile. Invoked on a background thread.
  // Mirrors the df::DrapeEngine::AddTileBackgroundImage + SetTileBackgroundData call pair:
  //   AddTileBackgroundImage(tileKey, imageUid, width, height, RGBA8, mode, std::move(rgba));
  //   SetTileBackgroundData(tileKey, imageUid, rect);
  using TReadyFn =
      std::function<void(df::TileKey const & tileKey, dp::BackgroundMode mode, std::string const & imageUid,
                         uint32_t width, uint32_t height, m2::RectF const & rect, std::vector<uint8_t> && rgba)>;
  using TCancelFn = std::function<void()>;
  // Injectable transport start function. Production uses HttpClient; tests keep and complete the
  // handlers in a controlled order to exercise cancellation and concurrent completion races.
  using TDownloadFn =
      std::function<TCancelFn(std::string const & url, platform::HttpClient::CompletionHandler handler)>;

  RasterTileProvider(Params params, TReadyFn onReady, TDownloadFn downloadFn = {});

  // Called on the render thread for every newly visible tile. Schedules cache reads on the File
  // thread and starts asynchronous downloads on a cache miss.
  bool RequestTile(df::TileKey const & tileKey, dp::BackgroundMode mode);

  // Called on the render thread when a tile scrolls out of view; drops a not-yet-delivered
  // request so it is neither bound nor uploaded once it finishes.
  void CancelTile(df::TileKey const & tileKey, dp::BackgroundMode mode);

  // Live-updates m_params.m_urlTemplate and m_params.m_maxCacheBytes (thread-safe). When the URL
  // changes the on-disk cache is cleared, since cached z/x/y now map to a different source.
  void Reconfigure(std::string urlTemplate, uint64_t maxCacheBytes);

  // Source web-mercator XYZ tile plus the UV sub-rect within it that the given OM tile maps to.
  struct SourceTile
  {
    int m_z = -1;
    int m_x, m_y;
    m2::RectF m_rect{0, 0, 1, 1};
    bool IsValid() const { return m_z >= 0; }
    // "z/x/y": image dedup key handed to drape (over-zoom siblings share their ancestor's uid).
    std::string GetUid() const;
    // "z_x_y.tile": on-disk cache key / file name (format-neutral extension; stb sniffs the content).
    std::string GetFileName() const;
  };

  // Maps an OM tile (x, y, zoomLevel) onto its web-mercator XYZ tile. Pure function (no provider
  // state) so it can be unit-tested directly. The OM tile's m_x/m_y MUST be in the 2^(m_zoomLevel-1)
  // grid; feeding indices from a coarser (clamped) grid yields a wrong web tile. Tiles below minZoom
  // or outside the vertical world extent come back with IsValid() == false; tiles deeper than maxZoom
  // sample a sub-rect of the maxZoom ancestor (m_rect).
  static SourceTile ToSourceTile(df::TileKey const & tileKey, int minZoom, int maxZoom);

private:
  // Starts a non-blocking HTTP request; its completion handler decodes, caches and delivers the tile.
  void StartDownload(df::TileKey const & tileKey, dp::BackgroundMode mode, std::string const & url,
                     SourceTile const & src, uint64_t requestId);

  bool IsActive(df::TileKey const & tileKey, uint64_t requestId) const;
  // Removes a specific request instance. An older completion must not remove a replacement request
  // for the same coordinate.
  bool DropActive(df::TileKey const & tileKey, uint64_t requestId);
  void AttachCancel(df::TileKey const & tileKey, uint64_t requestId, TCancelFn cancelFn);

  // Scans the cache dir (posted to the File thread from the constructor, ahead of any cache read)
  // and seeds the LRU order by file time.
  void InitDiskCache();
  // Marks a cached tile (by file name) as most-recently-used (on a cache hit).
  void TouchCacheEntry(std::string const & name);
  // Registers a freshly-written tile (by file name), then evicts least-recently-used files over the cap.
  void AddCacheEntry(std::string const & name, uint64_t size);
  // Evicts oldest entries until the total is within the cap. Caller must hold m_cacheMutex.
  void EvictDiskCacheLocked();
  // Deletes every cached file and resets the index. Caller must hold m_cacheMutex.
  void ClearDiskCacheLocked();

  static void LogConfig(std::string const & urlTemplate, uint64_t maxCacheBytes);

  // Non-const: Reconfigure() mutates m_urlTemplate (under m_activeMutex) and m_maxCacheBytes
  // (under m_cacheMutex). All other fields are immutable after construction.
  Params m_params;
  TReadyFn const m_onReady;
  TDownloadFn const m_downloadFn;
  std::string m_cacheDir;

  mutable std::mutex m_activeMutex;
  struct ActiveRequest
  {
    uint64_t m_id = 0;
    uint64_t m_clientGeneration = 0;
    TCancelFn m_cancelFn;
  };
  std::unordered_map<df::TileKey, ActiveRequest> m_active;
  uint64_t m_nextRequestId = 0;

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
