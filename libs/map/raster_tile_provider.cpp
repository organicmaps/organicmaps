#include "map/raster_tile_provider.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"
#include "base/logging.hpp"

#include "3party/stb_image/stb_image.h"

#include <algorithm>
#include <atomic>
#include <string_view>
#include <vector>

namespace
{
double constexpr kRequestTimeoutSec = 10.0;

// XYZ raster PNGs store row 0 = north (image top). The tile-background shader maps the quad's
// south edge (mercator minY) to texture v = 0, and a sampler's v = 0 returns the first uploaded
// row on both GL and Metal. So we reverse rows here to make uploaded row 0 == tile south edge.
bool constexpr kFlipVertically = true;

std::string SubstituteZXY(std::string tmpl, int z, int x, int y)
{
  auto const replace = [&tmpl](std::string_view from, int value)
  {
    auto const pos = tmpl.find(from);
    if (pos != std::string::npos)
      tmpl.replace(pos, from.size(), std::to_string(value));
  };
  replace("{z}", z);
  replace("{x}", x);
  replace("{y}", y);
  return tmpl;
}

df::TileKey CoordinateKey(df::TileKey const & key)
{
  return {key.m_x, key.m_y, key.m_zoomLevel};
}

// Packs an stb-decoded (top-first) RGBA buffer into a tightly-packed buffer, reversing rows when
// kFlipVertically so uploaded row 0 == the tile's south edge. Takes ownership of and frees `data`.
bool FlipAndPack(stbi_uc * data, int w, int h, std::vector<uint8_t> & rgba, uint32_t & width, uint32_t & height)
{
  if (data == nullptr || w <= 0 || h <= 0)
  {
    if (data != nullptr)
      stbi_image_free(data);
    return false;
  }

  width = static_cast<uint32_t>(w);
  height = static_cast<uint32_t>(h);
  size_t const rowBytes = static_cast<size_t>(w) * 4;
  rgba.resize(rowBytes * static_cast<size_t>(h));
  for (int row = 0; row < h; ++row)
  {
    int const srcRow = kFlipVertically ? (h - 1 - row) : row;
    std::copy_n(data + static_cast<size_t>(srcRow) * rowBytes, rowBytes,
                rgba.data() + static_cast<size_t>(row) * rowBytes);
  }
  stbi_image_free(data);
  return true;
}

// Cache-hit path: decode a cached tile (PNG/JPEG) straight from disk (false if absent/unreadable).
bool DecodeFileToRGBA(std::string const & path, std::vector<uint8_t> & rgba, uint32_t & width, uint32_t & height)
{
  int w = 0, h = 0, comp = 0;
  stbi_uc * data = stbi_load(path.c_str(), &w, &h, &comp, 4 /* desired RGBA channels */);
  return FlipAndPack(data, w, h, rgba, width, height);
}

// Freshly-downloaded path: decode encoded PNG/JPEG bytes held in memory.
bool DecodeMemoryToRGBA(char const * bytes, size_t size, std::vector<uint8_t> & rgba, uint32_t & width,
                        uint32_t & height)
{
  int w = 0, h = 0, comp = 0;
  stbi_uc * data =
      stbi_load_from_memory(reinterpret_cast<stbi_uc const *>(bytes), static_cast<int>(size), &w, &h, &comp, 4);
  return FlipAndPack(data, w, h, rgba, width, height);
}

// Best-effort write of the raw encoded tile (PNG/JPEG) to the disk cache (temp + atomic rename).
// Returns true if the file is now in place.
bool WriteTileCache(std::string const & cachePath, char const * bytes, size_t size)
{
  static std::atomic<uint64_t> counter{0};
  std::string const tmp = cachePath + ".tmp" + std::to_string(counter.fetch_add(1));
  try
  {
    {
      FileWriter writer(tmp);
      writer.Write(bytes, size);
    }
    base::RenameFileX(tmp, cachePath);
  }
  catch (RootException const & e)
  {
    LOG(LWARNING, ("Cache write failed", cachePath, e.what()));
    base::DeleteFileX(tmp);
    return false;
  }
  return true;
}
}  // namespace

RasterTileProvider::RasterTileProvider(Params params, TReadyFn onReady, TDownloadFn downloadFn)
  : m_params(std::move(params))
  , m_onReady(std::move(onReady))
  , m_downloadFn(std::move(downloadFn))
{
  CHECK(m_onReady != nullptr, ());

  m_cacheDir = GetPlatform().WritableDir() + m_params.m_cacheSubdir + "/";
  if (!Platform::MkDirRecursively(m_cacheDir))
    LOG(LWARNING, ("Failed to create cache dir", m_cacheDir));

  LogConfig(m_params.m_urlTemplate, m_params.m_maxCacheBytes);

  // The scan stats every cached file — run it on the File thread instead of stalling the caller.
  // RequestTile's cache reads are posted to the same (serial) File queue, so they run after it.
  GetPlatform().RunTask(Platform::Thread::File, [this]() { InitDiskCache(); });
}

std::string RasterTileProvider::SourceTile::GetUid() const
{
  return std::to_string(m_z) + "/" + std::to_string(m_x) + "/" + std::to_string(m_y);
}

std::string RasterTileProvider::SourceTile::GetFileName() const
{
  // Format-neutral extension: the bytes may be PNG or JPEG depending on the endpoint, and stb_image
  // sniffs the actual content on decode.
  return std::to_string(m_z) + "_" + std::to_string(m_x) + "_" + std::to_string(m_y) + ".tile";
}

RasterTileProvider::SourceTile RasterTileProvider::ToSourceTile(df::TileKey const & tileKey, int minZoom, int maxZoom)
{
  SourceTile src;

  // OM splits the world into 2^(Z-1) tiles per axis at zoomLevel Z (the "-1" is scales::INITIAL_LEVEL,
  // see scales.cpp), so the 0-based web-mercator zoom is Z-1.
  int const z = static_cast<int>(tileKey.m_zoomLevel) - 1;
  if (z < 1 || z < minZoom)
    return src;  // degenerate or below the configured minimum zoom

  int const n = 1 << z;
  // OM tile (0,0) corner sits at mercator (0,0) == lon 0 / lat 0; X grows east, Y grows north.
  // XYZ origin is the NW world corner with Y growing south, so shift X by a half-world and flip Y.
  int const webX = ((tileKey.m_x + n / 2) % n + n) % n;  // wrap antimeridian copies into [0, n)
  int const webY = n / 2 - 1 - tileKey.m_y;
  if (webY < 0 || webY >= n)
    return src;  // outside the vertical world extent

  src.m_z = z;
  src.m_x = webX;
  src.m_y = webY;

  // Over-zoom: the server has no tiles deeper than maxZoom, so sample a sub-rect of the
  // ancestor tile at maxZoom. Many child OM tiles then share one downloaded image (deduped by uid).
  if (z > maxZoom)
  {
    int const k = z - maxZoom;
    int const f = 1 << k;
    int const subX = webX & (f - 1);
    int const subY = webY & (f - 1);  // subY == 0 is the northern-most child
    src.m_z = maxZoom;
    src.m_x = webX >> k;
    src.m_y = webY >> k;
    float const inv = 1.0f / static_cast<float>(f);
    // Rows are flipped to south-first, so v grows north; subY == 0 (north) maps to the high-v band.
    src.m_rect = m2::RectF(static_cast<float>(subX) * inv, 1.0f - static_cast<float>(subY + 1) * inv,
                           static_cast<float>(subX + 1) * inv, 1.0f - static_cast<float>(subY) * inv);
  }

  return src;
}

bool RasterTileProvider::RequestTile(df::TileKey const & tileKey, dp::BackgroundMode mode)
{
  SourceTile const src = ToSourceTile(tileKey, m_params.m_minZoom, m_params.m_maxZoom);
  if (!src.IsValid())
    return false;

  // Skip tiles that don't intersect the configured coverage box. Use the wrapped rect: extended
  // world copies past the antimeridian have lon outside [-180, 180] and would fail the box test
  // otherwise, although ToSourceTile maps them onto valid source tiles.
  // Unclipped: background tiles use the real zoom (vector data clamps at GetUpperScale(), which would
  // misplace the lat/lon box test above that zoom).
  m2::RectD const ll = mercator::ToLatLon(tileKey.GetWrappedDataRect(false /* clipByDataMaxZoom */));
  if (ll.maxX() < m_params.m_minLon || ll.minX() > m_params.m_maxLon || ll.maxY() < m_params.m_minLat ||
      ll.minY() > m_params.m_maxLat)
    return false;

  std::string urlTemplate;
  uint64_t requestId = 0;
  TCancelFn supersededCancel;
  {
    std::lock_guard lock(m_activeMutex);
    urlTemplate = m_params.m_urlTemplate;  // may be changed live by Reconfigure
    if (urlTemplate.empty())
      return false;  // not configured

    auto const activeKey = CoordinateKey(tileKey);
    auto const activeIt = m_active.find(activeKey);
    if (activeIt != m_active.end())
    {
      if (activeIt->second.m_clientGeneration == tileKey.m_generation)
        return true;  // this exact request is already in flight
      supersededCancel = std::move(activeIt->second.m_cancelFn);
      m_active.erase(activeIt);
    }

    requestId = ++m_nextRequestId;
    m_active.emplace(activeKey, ActiveRequest{requestId, tileKey.m_generation, {}});
  }
  if (supersededCancel)
    supersededCancel();

  std::string const url = SubstituteZXY(std::move(urlTemplate), src.m_z, src.m_x, src.m_y);

  // Read/decode the cache on the serial File queue; downloads are asynchronous on a miss.
  // Return true only if posted, so the renderer never awaits a request that cannot complete.
  auto const posted = GetPlatform().RunTask(Platform::Thread::File, [this, tileKey, mode, url, src, requestId]()
  {
    if (!IsActive(tileKey, requestId))
      return;

    std::vector<uint8_t> rgba;
    uint32_t width = 0, height = 0;
    auto const fileName = src.GetFileName();
    if (DecodeFileToRGBA(m_cacheDir + fileName, rgba, width, height))  // cache hit
    {
      TouchCacheEntry(fileName);
      if (DropActive(tileKey, requestId))
        m_onReady(tileKey, mode, src.GetUid(), width, height, src.m_rect, std::move(rgba));
      return;
    }

    // Cache miss (file absent or unreadable): start the async download.
    StartDownload(tileKey, mode, url, src, requestId);
  });

  if (!posted.m_isSuccess)
  {
    DropActive(tileKey, requestId);
    return false;
  }

  return true;
}

void RasterTileProvider::StartDownload(df::TileKey const & tileKey, dp::BackgroundMode mode, std::string const & url,
                                       SourceTile const & src, uint64_t requestId)
{
  platform::HttpClient::CompletionHandler handler =
      [this, tileKey, mode, src, requestId](platform::HttpClient::Result result)
  {
    if (!result.m_success || result.m_errorCode != 200)
    {
      if (!DropActive(tileKey, requestId))
        return;
      LOG(LWARNING, ("Failed to download", src.GetUid(), "code", result.m_errorCode));
      // An empty uid reports a failed read to the renderer.
      m_onReady(tileKey, mode, {} /* imageUid */, 0, 0, {}, {});
      return;
    }

    auto const & body = result.m_serverResponse;
    std::vector<uint8_t> rgba;
    uint32_t width = 0, height = 0;
    if (body.empty() || !DecodeMemoryToRGBA(body.data(), body.size(), rgba, width, height))
    {
      if (!DropActive(tileKey, requestId))
        return;
      LOG(LWARNING, ("Failed to decode", src.GetUid()));
      // Same terminal-failure report as the download path above.
      m_onReady(tileKey, mode, {} /* imageUid */, 0, 0, {}, {});
      return;
    }

    auto const fileName = src.GetFileName();
    if (WriteTileCache(m_cacheDir + fileName, body.data(), body.size()))
      AddCacheEntry(fileName, body.size());

    // Deliver only if the request was not cancelled while it was in flight.
    if (DropActive(tileKey, requestId))
      m_onReady(tileKey, mode, src.GetUid(), width, height, src.m_rect, std::move(rgba));
  };

  TCancelFn cancelFn;
  if (m_downloadFn)
  {
    cancelFn = m_downloadFn(url, std::move(handler));
  }
  else
  {
    platform::HttpClient request(url);
    request.SetTimeout(kRequestTimeoutSec);
    request.SetFollowRedirects(true);
    auto handle = request.RunHttpRequestAsync(std::move(handler));
    cancelFn = [handle = std::move(handle)]() mutable { handle.Cancel(); };
  }
  AttachCancel(tileKey, requestId, std::move(cancelFn));
}

void RasterTileProvider::CancelTile(df::TileKey const & tileKey, dp::BackgroundMode /* mode */)
{
  TCancelFn cancelFn;
  {
    std::lock_guard lock(m_activeMutex);
    auto const it = m_active.find(CoordinateKey(tileKey));
    if (it == m_active.end() || it->second.m_clientGeneration != tileKey.m_generation)
      return;
    cancelFn = std::move(it->second.m_cancelFn);
    m_active.erase(it);
  }
  if (cancelFn)
    cancelFn();
}

void RasterTileProvider::InitDiskCache()
{
  struct ScannedFile
  {
    time_t m_time;
    std::string m_name;
    uint64_t m_size;
  };

  // GetFilesByRegExp returns base names, which are exactly the cache index keys.
  Platform::FilesList names;
  Platform::GetFilesByRegExp(m_cacheDir, ".*\\.tile$", names);

  std::vector<ScannedFile> files;
  files.reserve(names.size());
  for (auto & name : names)
  {
    std::string const fullPath = m_cacheDir + name;
    uint64_t size = 0;
    if (!Platform::GetFileSizeByFullPath(fullPath, size))
      continue;
    files.push_back({Platform::GetFileCreationTime(fullPath), std::move(name), size});
  }

  // Oldest first, so least-recently-modified files sit at the front (LRU end) of the list.
  std::sort(files.begin(), files.end(),
            [](ScannedFile const & a, ScannedFile const & b) { return a.m_time < b.m_time; });

  std::lock_guard lock(m_cacheMutex);
  // Downloads are started only from later tasks on the same (single-threaded) File queue, so
  // nothing can index a tile before this scan finishes.
  ASSERT(m_cacheIndex.empty(), ());
  for (auto & f : files)
  {
    m_lru.push_back(f.m_name);
    m_cacheIndex.emplace(std::move(f.m_name), CacheEntry{f.m_size, std::prev(m_lru.end())});
    m_cacheBytes += f.m_size;
  }
  EvictDiskCacheLocked();  // in case the cap was lowered between runs

  LOG(LINFO, ("Disk cache", m_cacheIndex.size(), "tiles,", m_cacheBytes / 1024, "KB (cap",
              m_params.m_maxCacheBytes / 1024, "KB)"));
}

void RasterTileProvider::TouchCacheEntry(std::string const & name)
{
  std::lock_guard lock(m_cacheMutex);
  auto const it = m_cacheIndex.find(name);
  if (it == m_cacheIndex.end())
    return;
  // Move the node to the back (most-recently-used). Splice keeps the stored iterator valid.
  m_lru.splice(m_lru.end(), m_lru, it->second.m_lruIt);
}

void RasterTileProvider::AddCacheEntry(std::string const & name, uint64_t size)
{
  std::lock_guard lock(m_cacheMutex);
  auto const it = m_cacheIndex.find(name);
  if (it != m_cacheIndex.end())
  {
    // Overwritten (re-download): adjust the accounted size and promote to most-recently-used.
    m_cacheBytes -= it->second.m_size;
    it->second.m_size = size;
    m_cacheBytes += size;
    m_lru.splice(m_lru.end(), m_lru, it->second.m_lruIt);
  }
  else
  {
    m_lru.push_back(name);
    m_cacheIndex.emplace(name, CacheEntry{size, std::prev(m_lru.end())});
    m_cacheBytes += size;
  }
  EvictDiskCacheLocked();
}

void RasterTileProvider::EvictDiskCacheLocked()
{
  while (m_cacheBytes > m_params.m_maxCacheBytes && !m_lru.empty())
  {
    std::string const victim = std::move(m_lru.front());
    m_lru.pop_front();

    auto const it = m_cacheIndex.find(victim);
    if (it != m_cacheIndex.end())
    {
      m_cacheBytes -= std::min(m_cacheBytes, it->second.m_size);
      m_cacheIndex.erase(it);
    }

    if (!base::DeleteFileX(m_cacheDir + victim))
      LOG(LWARNING, ("Failed to evict cache file", victim));
  }
}

void RasterTileProvider::ClearDiskCacheLocked()
{
  for (auto const & name : m_lru)
    base::DeleteFileX(m_cacheDir + name);
  m_lru.clear();
  m_cacheIndex.clear();
  m_cacheBytes = 0;
}

void RasterTileProvider::LogConfig(std::string const & urlTemplate, uint64_t maxCacheBytes)
{
  LOG(LINFO, ("URL =", urlTemplate.substr(0, urlTemplate.find('?')), "; cache size =", maxCacheBytes / 1024, "KB"));
}

void RasterTileProvider::Reconfigure(std::string urlTemplate, uint64_t maxCacheBytes)
{
  LogConfig(urlTemplate, maxCacheBytes);

  bool urlChanged = false;
  {
    std::lock_guard lock(m_activeMutex);  // guards m_params.m_urlTemplate (also read in RequestTile)
    urlChanged = urlTemplate != m_params.m_urlTemplate;
    m_params.m_urlTemplate = std::move(urlTemplate);
  }

  std::lock_guard lock(m_cacheMutex);
  m_params.m_maxCacheBytes = maxCacheBytes;

  // A different source reuses the same z/x/y file names for different imagery, so drop the cache.
  // Same source with a smaller cap just needs eviction down to the new limit.
  if (urlChanged)
    ClearDiskCacheLocked();
  else
    EvictDiskCacheLocked();
}

bool RasterTileProvider::IsActive(df::TileKey const & tileKey, uint64_t requestId) const
{
  std::lock_guard lock(m_activeMutex);
  auto const it = m_active.find(CoordinateKey(tileKey));
  return it != m_active.end() && it->second.m_id == requestId;
}

bool RasterTileProvider::DropActive(df::TileKey const & tileKey, uint64_t requestId)
{
  std::lock_guard lock(m_activeMutex);
  auto const it = m_active.find(CoordinateKey(tileKey));
  if (it == m_active.end() || it->second.m_id != requestId)
    return false;
  m_active.erase(it);
  return true;
}

void RasterTileProvider::AttachCancel(df::TileKey const & tileKey, uint64_t requestId, TCancelFn cancelFn)
{
  bool cancelImmediately = false;
  {
    std::lock_guard lock(m_activeMutex);
    auto const it = m_active.find(CoordinateKey(tileKey));
    if (it == m_active.end() || it->second.m_id != requestId)
      cancelImmediately = true;
    else
      it->second.m_cancelFn = std::move(cancelFn);
  }
  if (cancelImmediately && cancelFn)
    cancelFn();
}
