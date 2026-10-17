#pragma once

#include "drape_frontend/tile_key.hpp"

#include "drape/drape_global.hpp"

#include "platform/http_client.hpp"

#include "geometry/rect2d.hpp"

#include "base/macros.hpp"

#include <functional>
#include <memory>
#include <string>
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
// callback that the caller wires to df::DrapeEngine::AddTileBackgroundImage.

// Optional debug status imagery (DOWNLOADING / NOT FOUND / ERROR), disabled for production.
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

  // Delivers decoded RGBA8 pixels on the GUI thread, ready to upload and bind with
  // df::DrapeEngine::AddTileBackgroundImage.
  using TReadyFn =
      std::function<void(df::TileKey const & tileKey, dp::BackgroundMode mode, std::string const & imageUid,
                         uint32_t width, uint32_t height, m2::RectF const & rect, std::vector<uint8_t> && rgba)>;

  // Optional transport override for controlling asynchronous completions in tests.
  using TDownloadFn = std::function<void(std::string const & url, platform::HttpClient::CompletionHandler handler)>;

  // Construct, shut down and destroy on the GUI thread, which also owns onReady's recipient.
  RasterTileProvider(Params params, TReadyFn onReady, TDownloadFn downloadFn = {});
  ~RasterTileProvider();

  // Rejects new requests and suppresses queued deliveries. Call before stopping Platform's
  // worker threads, retaining the provider until rendering threads no longer call it.
  void Shutdown();

  // Called on the render thread for every newly visible tile. Non-blocking: schedules the
  // cache read/decode on the File thread and an asynchronous HTTP request on a cache miss.
  bool RequestTile(df::TileKey const & tileKey, dp::BackgroundMode mode);

  // Called on the render thread when a tile scrolls out of view; drops a not-yet-delivered
  // request so it is neither bound nor uploaded once it finishes.
  void CancelTile(df::TileKey const & tileKey, dp::BackgroundMode mode);

  // Called on the GUI thread. A URL change invalidates pending requests and returns true so the
  // caller can reset renderer bindings. Source-specific disk entries share the same cache cap.
  bool Reconfigure(std::string urlTemplate, uint64_t maxCacheBytes);

private:
  struct State;
  std::shared_ptr<State> const m_state;

  DISALLOW_COPY_AND_MOVE(RasterTileProvider);
};
