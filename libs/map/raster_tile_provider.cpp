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
// If the basemap renders vertically mirrored, flip this single constant.
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

#ifdef ENABLE_STATUS_PLACEHOLDERS
int constexpr kPlaceholderSize = 256;

// Minimal 8x8 bitmap font for A-Z (public-domain font8x8_basic subset), used to draw debug status
// text into placeholder tiles. Each glyph is 8 rows; within a row bit i (LSB first) is column i.
uint8_t const kFont8x8[26][8] = {
    {0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00},  // A
    {0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00},  // B
    {0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00},  // C
    {0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00},  // D
    {0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00},  // E
    {0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00},  // F
    {0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00},  // G
    {0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00},  // H
    {0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},  // I
    {0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00},  // J
    {0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00},  // K
    {0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00},  // L
    {0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00},  // M
    {0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00},  // N
    {0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00},  // O
    {0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00},  // P
    {0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00},  // Q
    {0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00},  // R
    {0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00},  // S
    {0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},  // T
    {0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00},  // U
    {0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00},  // V
    {0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00},  // W
    {0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00},  // X
    {0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00},  // Y
    {0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00},  // Z
};

// Sets one RGBA pixel, flipping the row when kFlipVertically (so the image matches the south-first
// upload order used for real tiles, keeping the drawn text upright on screen).
void PutPixel(std::vector<uint8_t> & img, int size, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  if (x < 0 || y < 0 || x >= size || y >= size)
    return;
  int const dstY = kFlipVertically ? size - 1 - y : y;
  size_t const idx = base::asserted_cast<size_t>(dstY * size + x) * 4;
  img[idx] = r;
  img[idx + 1] = g;
  img[idx + 2] = b;
  img[idx + 3] = a;
}

// Builds a square RGBA8 placeholder: a translucent colored fill with centered, upscaled white text.
std::vector<uint8_t> MakePlaceholder(std::string const & text, int size, uint8_t br, uint8_t bg, uint8_t bb, uint8_t ba)
{
  std::vector<uint8_t> img(size * size * 4);
  for (int i = 0; i < size * size; ++i)
  {
    img[i * 4] = br;
    img[i * 4 + 1] = bg;
    img[i * 4 + 2] = bb;
    img[i * 4 + 3] = ba;
  }

  // Scale 2 keeps the longest label ("DOWNLOADING", 11 chars * 16px = 176px) within the tile.
  int constexpr kScale = 2;
  int constexpr kCharW = 8 * kScale;
  int const textW = static_cast<int>(text.size()) * kCharW;
  int const x0 = (size - textW) / 2;
  int const y0 = (size - 8 * kScale) / 2;

  for (size_t ci = 0; ci < text.size(); ++ci)
  {
    char const c = text[ci];
    if (c < 'A' || c > 'Z')
      continue;  // space / unsupported -> blank
    uint8_t const * glyph = kFont8x8[c - 'A'];
    int const charX = x0 + static_cast<int>(ci) * kCharW;
    for (int gy = 0; gy < 8; ++gy)
    {
      for (int gx = 0; gx < 8; ++gx)
      {
        if (((glyph[gy] >> gx) & 1) == 0)
          continue;
        for (int sy = 0; sy < kScale; ++sy)
          for (int sx = 0; sx < kScale; ++sx)
            PutPixel(img, size, charX + gx * kScale + sx, y0 + gy * kScale + sy, 255, 255, 255, 255);
      }
    }
  }
  return img;
}
#endif  // ENABLE_STATUS_PLACEHOLDERS

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

#ifdef ENABLE_STATUS_PLACEHOLDERS
  // Translucent colored fills + white text; all 256x256 so they share the tiles' texture pool.
  m_placeholders[static_cast<size_t>(Status::Downloading)] = {
      "bgpoc:downloading", MakePlaceholder("DOWNLOADING", kPlaceholderSize, 30, 60, 120, 90), kPlaceholderSize};
  m_placeholders[static_cast<size_t>(Status::NotFound)] = {
      "bgpoc:notfound", MakePlaceholder("NOT FOUND", kPlaceholderSize, 120, 30, 30, 90), kPlaceholderSize};
  m_placeholders[static_cast<size_t>(Status::Error)] = {
      "bgpoc:error", MakePlaceholder("ERROR", kPlaceholderSize, 120, 60, 0, 110), kPlaceholderSize};
#endif

  LogConfig();

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

  // LOG(LDEBUG, (tileKey.Coord2String(), "-> XYZ", src.GetUid(), "rect", src.m_rect, url));

  // Disk cache read (and stb decode) runs on the File thread; on a miss the HTTP request is fully
  // async (see StartDownload), so no worker thread is ever parked waiting on the network.
  // Returning true from this method means the renderer may put tileKey into its awaiting set. That
  // is safe only if the task was actually posted: a later !IsActive() inside the task means the tile
  // was cancelled meanwhile, but a failed post means nothing can ever deliver or be cancelled.
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
#ifdef ENABLE_STATUS_PLACEHOLDERS
    DeliverPlaceholder(tileKey, mode, Status::Downloading);
#endif
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
      LOG(LWARNING, ("Failed to download", src.GetUid(), "code", result.m_errorCode));
#ifdef ENABLE_STATUS_PLACEHOLDERS
      if (DropActive(tileKey, requestId))
        DeliverPlaceholder(tileKey, mode, result.m_errorCode == 404 ? Status::NotFound : Status::Error);
#else
      // Report the failed read with an empty uid: the renderer must stop awaiting the tile, or it
      // would never be re-requested while it stays in the viewport (and the fallback level would
      // stay retained forever).
      if (DropActive(tileKey, requestId))
        m_onReady(tileKey, mode, {} /* imageUid */, 0, 0, {}, {});
#endif
      return;
    }

    auto const & body = result.m_serverResponse;
    std::vector<uint8_t> rgba;
    uint32_t width = 0, height = 0;
    if (body.empty() || !DecodeMemoryToRGBA(body.data(), body.size(), rgba, width, height))
    {
      LOG(LWARNING, ("Failed to decode", src.GetUid()));
#ifdef ENABLE_STATUS_PLACEHOLDERS
      if (DropActive(tileKey, requestId))
        DeliverPlaceholder(tileKey, mode, Status::Error);
#else
      // Same terminal-failure report as the download path above.
      if (DropActive(tileKey, requestId))
        m_onReady(tileKey, mode, {} /* imageUid */, 0, 0, {}, {});
#endif
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

void RasterTileProvider::LogConfig()
{
  LOG(LINFO, ("URL =", m_params.m_urlTemplate, "; cache size =", m_params.m_maxCacheBytes / 1024, "KB"));
}

void RasterTileProvider::Reconfigure(std::string urlTemplate, uint64_t maxCacheBytes)
{
  bool urlChanged = false;
  {
    std::lock_guard lock(m_activeMutex);  // guards m_params.m_urlTemplate (also read in RequestTile)
    urlChanged = urlTemplate != m_params.m_urlTemplate;
    m_params.m_urlTemplate = std::move(urlTemplate);
  }

  std::lock_guard lock(m_cacheMutex);
  m_params.m_maxCacheBytes = maxCacheBytes;

  LogConfig();

  // A different source reuses the same z/x/y file names for different imagery, so drop the cache.
  // Same source with a smaller cap just needs eviction down to the new limit.
  if (urlChanged)
    ClearDiskCacheLocked();
  else
    EvictDiskCacheLocked();
}

#ifdef ENABLE_STATUS_PLACEHOLDERS
void RasterTileProvider::DeliverPlaceholder(df::TileKey const & tileKey, dp::BackgroundMode mode, Status status)
{
  auto const & ph = m_placeholders[static_cast<size_t>(status)];
  // Copy the bytes: the engine moves them in and dedupes by uid across tiles.
  m_onReady(tileKey, mode, ph.m_uid, ph.m_size, ph.m_size, m2::RectF(0.0f, 0.0f, 1.0f, 1.0f),
            std::vector<uint8_t>(ph.m_rgba));
}
#endif

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
      it->second.m_cancelFn = cancelFn;
  }
  if (cancelImmediately && cancelFn)
    cancelFn();
}
